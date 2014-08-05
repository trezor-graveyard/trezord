#include <boost/network/include/http/server.hpp>
#include <boost/regex.hpp>

#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <locale>
#include <map>

namespace trezord
{
namespace api
{

// json support

typedef std::pair<std::string, Json::Value> json_pair;

Json::Value
json_value(std::initializer_list<json_pair> const &il)
{
    Json::Value json(Json::objectValue);
    for (auto const &kv: il) { json[kv.first] = kv.second; }
    return json;
}

std::string
json_string(Json::Value const &json)
{ return json.toStyledString(); }

std::string
json_string(std::initializer_list<json_pair> const &il)
{ return json_string(json_value(il)); }

// gathered response data

template <typename Status>
struct response_exception
    : public std::runtime_error
{
    Status status;

    response_exception(Status status_, std::string const &message)
        : status(status_),
          runtime_error(message)
    {}
};

template <typename Server>
struct response_data
{
    typedef Server server_type;

    typedef typename server_type::connection connection_type;
    typedef typename server_type::connection_ptr connection_ptr_type;
    typedef typename server_type::response_header header_type;
    typedef typename server_type::connection::status_t status_type;
    typedef response_exception<status_type> response_exception;

    status_type status;
    std::vector<header_type> headers;

    void
    set_header(std::string const &name, std::string const &value)
    {
        headers.push_back(header_type{name, value});
    }

    void
    write(connection_ptr_type connection, std::string const &body) const
    {
        CLOG(INFO, "http") << "<- " << status;
        connection->set_status(status);
        connection->set_headers(headers);
        connection->write(body);
    }

    void
    handle_error(connection_ptr_type connection, std::exception_ptr eptr)
    {
        try {
            if (eptr) {
                std::rethrow_exception(eptr);
            }
        }
        catch (response_exception const &e) {
            CLOG(ERROR, "http") << e.what();
            status = e.status;
            write(connection, json_string({{"error", e.what()}}));
        }
        catch (std::exception const &e) {
            CLOG(ERROR, "http") << e.what();
            status = connection_type::internal_server_error;
            write(connection, json_string({{"error", e.what()}}));
        }
        catch (...) {
            CLOG(ERROR, "http") << "unknown error";
            status = connection_type::internal_server_error;
            write(connection, json_string({{"error", "unknown error"}}));
        }
    }
};

// request handler

template <typename Server>
struct request_handler
{
    typedef Server server_type;

    typedef typename server_type::request request_type;
    typedef typename server_type::connection connection_type;
    typedef typename server_type::connection_ptr connection_ptr_type;
    typedef response_data<server_type> response_data_type;

    struct response_error
        : public response_data_type::response_exception
    { using response_data_type::response_exception::response_exception; };

    request_handler(request_handler const &) = delete;
    request_handler &operator=(request_handler const&) = delete;

    request_handler(core::kernel &kernel_)
        : kernel(kernel_),
          action_routes({
                  { "GET",  "/",             &request_handler::handle_index },
                  { "GET",  "/enumerate",    &request_handler::handle_enumerate },
                  { "POST", "/configure",    &request_handler::handle_configure },
                  { "POST", "/acquire/(.+)", &request_handler::handle_acquire },
                  { "POST", "/release/(.+)", &request_handler::handle_release },
                  { "POST", "/call/(.+)",    &request_handler::handle_call },
                  { ".*",   ".*",            &request_handler::handle_404 }})
    {}

    void
    operator()(request_type const &request,
               response_data_type response,
               connection_ptr_type connection)
    {
        response.set_header("Content-Type", "application/json");

        try {
            action_params params;
            action_handler ahandler;
            dispatch(request, ahandler, params);
            ahandler(this, params, request, response, connection);
        }
        catch (...) {
            response.handle_error(connection, std::current_exception());
        }
    }

    bool
    is_url_allowed(std::string const &url)
    {
        return kernel.is_allowed(url);
    }

private:

    typedef boost::smatch action_params;
    typedef std::function<
        void (request_handler*,
              action_params const &,
              request_type const &,
              response_data_type,
              connection_ptr_type)
        > action_handler;

    struct action_route
    {
        const boost::regex method;
        const boost::regex destination;
        const action_handler handler;

        action_route(std::string const &m,
                     std::string const &d,
                     action_handler const &h)
            : method(m),
              destination(d),
              handler(h) {}
    };

    std::vector<action_route> action_routes;

    void
    dispatch(request_type const &request,
             action_handler &handler,
             action_params &params)
    {
        for (auto const &route: action_routes) {
            bool m = boost::regex_match(request.method, route.method);
            bool d = boost::regex_match(request.destination, params,
                                        route.destination);
            if (m && d) {
                handler = route.handler;
                return;
            }
        }
        throw std::invalid_argument("route not found");
    }

    // handlers

    core::kernel &kernel;

    void
    handle_index(action_params const &params,
                 request_type const &request,
                 response_data_type response,
                 connection_ptr_type connection)
    {
        response.status = connection_type::ok;
        response.write(connection,
                       json_string({
                               {"configured", kernel.is_configured()},
                               {"version", kernel.get_version()}
                           }));
    }

    void
    handle_configure(action_params const &params,
                     request_type const &request,
                     response_data_type response,
                     connection_ptr_type connection)
    {
        core::kernel_config config;

        try {
            config.parse_from_signed_string(
                utils::hex_decode(request.body));
        }
        catch (std::exception const &e) {
            throw response_error(
                connection_type::bad_request, e.what());
        }

        LOG(INFO)
            << "parsed configuration: " << std::endl
            << config.get_debug_string();

        if (!config.is_initialized()) {
            throw response_error(
                connection_type::bad_request,
                "configuration is incomplete");
        }

        if (!config.is_unexpired()) {
            throw response_error(
                connection_type::bad_request,
                "configuration is expired");
        }

        auto origin_header_it = std::find_if(
            request.headers.begin(),
            request.headers.end(),
            [&] (typename request_type::header_type const &header) {
                return header.name == "Origin";
            });

        if (origin_header_it != request.headers.end()) {
            auto &origin = origin_header_it->value;
            if (!config.is_url_allowed(origin)) {
                throw response_error(
                    connection_type::forbidden,
                    "origin not allowed by given config");
            }
        }

        kernel.set_config(config);

        response.status = connection_type::ok;
        response.write(connection, json_string({}));
    }

    void
    handle_enumerate(action_params const &params,
                     request_type const &request,
                     response_data_type response,
                     connection_ptr_type connection)
    {
        kernel.get_enumeration_executor()->add(
            [=] () mutable {
                try {
                    auto devices = kernel.enumerate_devices();

                    Json::Value nil;
                    Json::Value item(Json::objectValue);
                    Json::Value list(Json::arrayValue);

                    for (auto const &d: devices) {
                        auto const &i = d.first;
                        auto const &s = d.second;

                        item["path"] = i.path;
                        item["vendor"] = i.vendor_id;
                        item["product"] = i.product_id;
                        item["serialNumber"] = std::string(
                            i.serial_number.begin(),
                            i.serial_number.end());
                        item["session"] = s.empty() ? nil : s;
                        list.append(item);
                    }

                    response.status = connection_type::ok;
                    response.write(connection, json_string(list));
                }
                catch (...) {
                    response.handle_error(connection, std::current_exception());
                }
            });
    }

    void
    handle_acquire(action_params const &params,
                   request_type const &request,
                   response_data_type response,
                   connection_ptr_type connection)
    {
        auto device_path = params.str(1);

        auto acquisition = [=] () mutable {
            try {
                auto device = kernel.get_device_kernel(device_path);
                device->open();
                auto session_id = kernel.acquire_session(device_path);

                response.status = connection_type::ok;
                response.write(connection,
                               json_string({
                                       {"session", session_id}
                                   }));
            }
            catch (...) {
                response.handle_error(connection, std::current_exception());
            }
        };

        kernel.get_enumeration_executor()->add(
            [=] () mutable {
                try {
                    if (!kernel.is_path_supported(device_path)) {
                        throw response_error(
                            connection_type::forbidden,
                            "device not found or unsupported");
                    }

                    kernel.get_device_executor(device_path)->add(acquisition);
                }
                catch (...) {
                    response.handle_error(connection, std::current_exception());
                }
            });
    }

    void
    handle_release(action_params const &params,
                   request_type const &request,
                   response_data_type response,
                   connection_ptr_type connection)
    {
        auto session_id = params.str(1);

        core::device_kernel *device;
        core::async_executor *executor;

        try {
            device = kernel.get_device_kernel_by_session_id(session_id);
            executor = kernel.get_device_executor_by_session_id(session_id);
        }
        catch (core::kernel::unknown_session const &e) {
            throw response_error(
                connection_type::not_found, e.what());
        }

        executor->add(
            [=] () mutable {
                try {
                    device->close();
                    kernel.release_session(session_id);

                    response.status = connection_type::ok;
                    response.write(connection, json_string({}));
                }
                catch (...) {
                    response.handle_error(connection, std::current_exception());
                }
            });
    }

    void
    handle_call(action_params const &params,
                request_type const &request,
                response_data_type response,
                connection_ptr_type connection)
    {
        auto session_id = params.str(1);

        core::device_kernel *device;
        core::async_executor *executor;

        try {
            device = kernel.get_device_kernel_by_session_id(session_id);
            executor = kernel.get_device_executor_by_session_id(session_id);
        }
        catch (core::kernel::unknown_session const &e) {
            throw response_error(
                connection_type::not_found, e.what());
        }

        executor->add(
            [=] () mutable {
                try {
                    wire::message wire_in;
                    wire::message wire_out;

                    Json::Value json;
                    Json::Reader json_reader;
                    json_reader.parse(request.body, json);

                    kernel.json_to_wire(json, wire_in);
                    device->call(wire_in, wire_out);
                    kernel.wire_to_json(wire_out, json);

                    response.status = connection_type::ok;
                    response.write(connection, json_string(json));
                }
                catch (...) {
                    response.handle_error(connection, std::current_exception());
                }
            });
    }

    void
    handle_404(action_params const &params,
               request_type const &request,
               response_data_type response,
               connection_ptr_type connection)
    {
        throw response_error(
            connection_type::not_found,
            "not found");
    }
};

template <typename Server, typename Handler>
struct body_middleware
    : boost::enable_shared_from_this< body_middleware<Server, Handler> >
{
    typedef Server server_type;
    typedef Handler handler_type;

    typedef typename server_type::request request_type;
    typedef typename server_type::connection connection_type;
    typedef typename server_type::connection_ptr connection_ptr_type;
    typedef response_data<server_type> response_data_type;

    body_middleware(body_middleware const &) = delete;
    body_middleware &operator=(body_middleware const&) = delete;

    body_middleware(handler_type &handler_)
        : handler(handler_),
          body()
    {}

    void
    operator()(request_type const &request,
               response_data_type response,
               connection_ptr_type connection)
    {
        body_size = 0;
        body_max_size = read_content_length(request);
        read_body(request, response, connection);
    }

private:

    handler_type &handler;
    std::stringstream body;
    std::size_t body_size;
    std::size_t body_max_size;

    void
    read_body(request_type const &request,
              response_data_type response,
              connection_ptr_type connection)
    {
        connection->read(
            boost::bind(
                &body_middleware<Server, Handler>::handle_body_read,
                body_middleware<Server, Handler>::shared_from_this(),
                _1, _2, _3, _4, request, response));
    }

    void
    handle_body_read(typename connection_type::input_range range,
                     boost::system::error_code error,
                     std::size_t size,
                     connection_ptr_type connection,
                     request_type const &request,
                     response_data_type response)
    {
        body.write(range.begin(), size);
        body_size += size;

        CLOG(DEBUG, "http.body")
            << body_size << "B/" << body_max_size << "B";

        if (body_size < body_max_size) {
            read_body(request, response, connection);
        } else {
            request.body = body.str();
            handler(request, response, connection);
        }
    }

    std::size_t
    read_content_length(request_type const &request)
    {
        auto content_length_header_it = std::find_if(
            request.headers.begin(),
            request.headers.end(),
            [&] (typename request_type::header_type const &header) {
                return header.name == "Content-Length";
            });

        if (content_length_header_it == request.headers.end()) {
            return 0;
        }

        return boost::lexical_cast<
            std::size_t
            >(content_length_header_it->value);
    }
};

template <typename Server,
          typename Handler,
          typename Middleware>
struct middleware_factory
{
    typedef Server server_type;
    typedef Handler handler_type;
    typedef Middleware middleware_type;

    typedef typename server_type::request request_type;
    typedef typename server_type::connection_ptr connection_ptr_type;
    typedef response_data<server_type> response_data_type;

    middleware_factory(handler_type &handler_)
        : handler(handler_)
    {}

    void
    operator()(request_type const &request,
               response_data_type response,
               connection_ptr_type connection)
    {
        auto middleware = boost::make_shared<middleware_type>(handler);
        (*middleware)(request, response, connection);
    }

private:

    handler_type &handler;
};

template <typename Server, typename Handler>
struct cors_middleware
{
    typedef Server server_type;
    typedef Handler handler_type;

    typedef std::function<bool(std::string const &)> origin_validator_type;

    typedef typename server_type::request request_type;
    typedef typename server_type::connection connection_type;
    typedef typename server_type::connection_ptr connection_ptr_type;
    typedef response_data<server_type> response_data_type;

    cors_middleware(handler_type &handler_,
                    origin_validator_type validator_)
        : handler(handler_),
          validator(validator_)
    {}

    void
    operator()(request_type const &request,
               response_data_type response,
               connection_ptr_type connection)
    {
        auto origin_header_it = find_header(request, "Origin");

        if (origin_header_it == request.headers.end()) {
            CLOG(INFO, "http.cors") << "non-cors accepted";
            handler(request, response, connection);
            return;
        }
        auto &origin = origin_header_it->value;

        if (!validator(origin)) {
            CLOG(WARNING, "http.cors") << "invalid origin refused: " << origin;

            response.status = connection_type::forbidden;
            response.write(connection, "Invalid Origin");
            return;
        }

        if (request.method == "OPTIONS") {
            CLOG(INFO, "http.cors") << "pre-flight accepted";

            project_header(request, "Access-Control-Request-Method",
                           response, "Access-Control-Allow-Methods");
            project_header(request, "Access-Control-Request-Headers",
                           response, "Access-Control-Allow-Headers");
            response.set_header("Access-Control-Allow-Origin", origin);
            response.status = connection_type::ok;
            response.write(connection, "");

        } else {
            CLOG(INFO, "http.cors") << "accepted";

            response.set_header("Access-Control-Allow-Origin", origin);
            handler(request, response, connection);
        }

    }

private:

    bool
    project_header(request_type const &request,
                   std::string const &request_name,
                   response_data_type &response,
                   std::string const &response_name)
    {
        auto header_it = find_header(request, request_name);
        if (header_it != request.headers.end()) {
            response.set_header(response_name, header_it->value);
            return true;
        }
        return false;
    }

    auto
    find_header(request_type const &request,
                std::string const &name)
        -> typename decltype(request.headers)::iterator
    {
        return std::find_if(
            request.headers.begin(),
            request.headers.end(),
            [&] (typename request_type::header_type const &header) {
                return header.name == name;
            });
    }

    handler_type &handler;
    origin_validator_type validator;
};

struct connection_handler
{
    typedef boost::network::http::async_server<
        connection_handler
        > server;

    typedef request_handler<server> handler_type;
    typedef response_data<server> response_data_type;

    connection_handler(handler_type &handler)
        : mw_body(handler),
          mw_cors(mw_body, boost::bind(
                      &handler_type::is_url_allowed,
                      &handler, _1))
    {}

    void
    operator()(server::request const &request,
               server::connection_ptr connection)
    {
        CLOG(INFO, "http")
            << "-> " << request.method
            << " " << request.destination;

        response_data_type response;
        mw_cors(request, response, connection);
    }

private:

    typedef middleware_factory<
        server, handler_type, body_middleware<server, handler_type>
        > body_middleware_type;

    typedef cors_middleware<
        server, body_middleware_type
        > cors_middleware_type;

    body_middleware_type mw_body;
    cors_middleware_type mw_cors;
};

}
}
