/*
 * This file is part of the TREZOR project.
 *
 * Copyright (C) 2014 SatoshiLabs
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <microhttpd.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

namespace trezord
{
namespace http_server
{

struct request_data
{
    MHD_Connection *connection;
    std::string url;
    std::string method;
    std::stringstream body;
    boost::smatch url_params;

    char const *
    get_header(char const *name) const
    {
        return MHD_lookup_connection_value(
            connection, MHD_HEADER_KIND, name);
    }
};

struct response_data
{
    using ptr = std::unique_ptr<
        MHD_Response, decltype(&MHD_destroy_response)>;

    int status_code;
    ptr response;

    response_data(int status, std::string const &body)
        : status_code{status},
          response{mhd_response_from_string(body.c_str()), &MHD_destroy_response}
    { }

    response_data(int status, char const *body)
        : status_code{status},
          response{mhd_response_from_string(body), &MHD_destroy_response}
    { }

    int
    add_header(char const *name, char const *value)
    {
        if (name && value) {
            return MHD_add_response_header(response.get(), name, value);
        }
        else {
            return MHD_NO;
        }
    }

    int
    respond_to(request_data *request)
    {
        if (request) {
            return MHD_queue_response(
                request->connection, status_code, response.get());
        }
        else {
            return MHD_NO;
        }
    }

private:

    static
    MHD_Response *
    mhd_response_from_string(char const *body)
    {
        // MHD_create_response_from_buffer has many modes of operation,
        // but the buffer for MHD_RESPMEM_MUST_COPY mode is effectively
        // const char *, so this conversion is safe
        auto body_buffer = static_cast<void *>(const_cast<char *>(body));

        return MHD_create_response_from_buffer(
            std::strlen(body), body_buffer, MHD_RESPMEM_MUST_COPY);
    }
};

using request_handler = std::function<response_data (request_data const &)>;

struct regex_route
{
    boost::regex method;
    boost::regex url;

    regex_route(char const *method_pattern,
                char const *url_pattern)
        : method{method_pattern},
          url{url_pattern}
    { }

    bool
    match_request(request_data *request) const
    {
        return (boost::regex_match(request->method, method))
            && (boost::regex_match(request->url, request->url_params, url));
    }
};

using route_entry = std::pair<regex_route, request_handler>;
using route_table = std::vector<route_entry>;
using cors_validator = std::function<bool(char const *)>;

response_data
handle_cors_and_delegate(cors_validator validator,
                         request_handler handler,
                         request_data const &request)
{
    auto origin = request.get_header("Origin");

    if (!origin) {
        // not a cors request, delegate
        return handler(request);
    }

    if (!validator(origin)) {
        // origin is not allowed, forbid further processing
        response_data response{403, "Origin Not Allowed"};
        return response;
    }

    if (request.method == "OPTIONS") {
        // allowed pre-flight request, thumbs up
        response_data response{200, "Enjoy Your Flight"};
        auto req_method = request.get_header("Access-Control-Request-Method");
        auto req_headers = request.get_header("Access-Control-Request-Headers");
        response.add_header("Access-Control-Allow-Methods", req_method);
        response.add_header("Access-Control-Allow-Headers", req_headers);
        response.add_header("Access-Control-Allow-Origin", origin);
        return response;
    }
    else {
        // allowed ordinary request, delegate
        response_data response = handler(request);
        response.add_header("Access-Control-Allow-Origin", origin);
        return response;
    }
}

struct server
{
    route_table const &routes;
    cors_validator validator;

    server(route_table const &table, cors_validator cors)
        : routes(table),
          validator{cors}
    { }

    ~server() { stop(); }

    void
    start(unsigned int port,
          char const *address,
          char const *key,
          char const *cert)
    {
        sockaddr_in addr;

        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, address, &addr.sin_addr);

        MHD_set_panic_func(&server::panic_callback, this);

        daemon = MHD_start_daemon(
            MHD_USE_THREAD_PER_CONNECTION | MHD_USE_SSL,
            port,
            nullptr, nullptr,
            &server::request_callback, this,
            MHD_OPTION_SOCK_ADDR, &addr,
            MHD_OPTION_HTTPS_MEM_KEY, key,
            MHD_OPTION_HTTPS_MEM_CERT, cert,
            MHD_OPTION_NOTIFY_COMPLETED, &server::completed_callback, this,
            MHD_OPTION_EXTERNAL_LOGGER, &server::log_callback, this,
            MHD_OPTION_CONNECTION_TIMEOUT, 0,
            MHD_OPTION_END);

        if (daemon) {
            CLOG(INFO, "http.server")
                << "listening at https://" << address << ":" << port;
        }
        else {
            throw std::runtime_error{"failed to start server"};
        }
    }

    void
    stop()
    {
        if (daemon) {
            MHD_stop_daemon(daemon);
            daemon = nullptr;
        }
    }

private:

    MHD_Daemon *daemon = nullptr;

    static
    int
    request_callback(void *cls,
                     MHD_Connection *connection,
                     char const *url,
                     char const *method,
                     char const *version,
                     char const *upload_data,
                     std::size_t *upload_data_size,
                     void **con_cls)
    {
        server *self = static_cast<server *>(cls);
        request_data *request = static_cast<request_data *>(*con_cls);

        if (!request) {
            request = new request_data{
                connection,
                url,
                method,
                std::stringstream(),
                boost::smatch()
            };
            *con_cls = request;
            return MHD_YES;
        }

        try {
            // buffer body data

            if (*upload_data_size > 0) {
                request->body.write(upload_data, *upload_data_size);
                *upload_data_size = 0;
                return MHD_YES;
            }

            CLOG(INFO, "http.server") << "<- " << method << " " << url;

            // find matching request handler

            request_handler handler = nullptr;

            for (auto const &r : self->routes) {
                if (r.first.match_request(request)) {
                    handler = r.second;
                    break;
                }
            }

            // handle the request

            if (handler) {
                auto response = handle_cors_and_delegate(
                    self->validator, handler, *request);
                CLOG(INFO, "http.server") << "-> " << response.status_code;
                return response.respond_to(request);
            }
            else {
                return MHD_NO;
            }
        }
        catch (...) {
            return MHD_NO;
        }
    }

    static
    void
    completed_callback(void *cls,
                       MHD_Connection *connection,
                       void **con_cls,
                       MHD_RequestTerminationCode toe)
    {
        request_data *request = static_cast<request_data *>(*con_cls);

        if (request) {
            delete request;
        }
    }

    static
    void
    panic_callback(void *cls, char const *file, unsigned int line, char const *reason)
    {
        CLOG(FATAL, "http.server") << file << ":" << line << ": " << reason;
    }

    static
    void
    log_callback(void *cls, char const *fm, va_list ap)
    {
        char message[4096];
        std::vsnprintf(message, sizeof(message), fm, ap);
        CLOG(INFO, "http.server") << message;
    }
};

}
}
