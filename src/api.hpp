#include <boost/network/include/http/server.hpp>

#include <functional>
#include <iostream>
#include <string>
#include <regex>
#include <map>

namespace trezord
{
namespace api
{

template <typename Server>
struct request_handler
{
    typedef Server server_type;

    typedef typename server_type::request request_type;
    typedef typename server_type::connection connection_type;
    typedef typename server_type::connection_ptr connection_ptr_type;

    request_handler(request_handler const &) = delete;
    request_handler &operator=(request_handler const&) = delete;

    request_handler(core::kernel &kernel_)
        : kernel(kernel_)
    {}

    void
    operator()(request_type const &request,
               connection_ptr_type connection)
    {
        try {
            action_params params;
            action_handler ahandler;

            dispatch(request, ahandler, params);
            ahandler(this, params, request, connection);
        }
        catch (std::exception const &e) {
            connection->set_status(connection_type::internal_server_error);
            connection->set_headers(json_headers);
            connection->write(json_string({{"error", e.what()}}));
        }
    }

private:

    core::kernel &kernel;

    // json support

    typedef std::pair<
        std::string,
        Json::Value
        > json_pair;

    typedef std::initializer_list<
        json_pair
        > json_list;

    const std::array<
        typename server_type::response_header, 1
        > json_headers = {{{"Content-Type", "application/json"}}};

    Json::Value
    json_value(json_list const &list)
    {
        Json::Value json(Json::objectValue);

        for (auto const &kv: list) {
            json[kv.first] = kv.second;
        }

        return json;
    }

    std::string
    json_string(Json::Value const &json)
    {
        return json.toStyledString();
    }

    std::string
    json_string(json_list const &list)
    {
        return json_string(json_value(list));
    }

    // dispatching

    typedef std::smatch action_params;
    typedef std::function<
    void (request_handler*,
          action_params const &,
          request_type const &,
          connection_ptr_type)
    > action_handler;

    struct action_route
    {
        const std::regex method;
        const std::regex destination;
        const action_handler handler;

        action_route(std::string const &m,
                     std::string const &d,
                     action_handler const &h)
        : method(m),
          destination(d),
          handler(h) {}
    };

    const action_route action_routes[7] = {
        { "GET",  "/",             &request_handler::handle_index },
        { "GET",  "/enumerate",    &request_handler::handle_enumerate },
        { "POST", "/configure",    &request_handler::handle_configure },
        { "POST", "/acquire/(.+)", &request_handler::handle_acquire },
        { "POST", "/release/(.+)", &request_handler::handle_release },
        { "POST", "/call/(.+)",    &request_handler::handle_call },
        { ".*",   ".*",            &request_handler::handle_404 },
    };

    void
    dispatch(request_type const &request,
             action_handler &handler,
             action_params &params)
    {
        for (auto const &route: action_routes) {
            bool m = std::regex_match(request.method, route.method);
            bool d = std::regex_match(request.destination, params,
                                      route.destination);
            if (m && d) {
                handler = route.handler;
                return;
            }
        }
        throw std::invalid_argument("route not found");
    }

    // handlers

    void
    handle_index(action_params const &params,
                 request_type const &request,
                 connection_ptr_type connection)
    {
        connection->set_status(connection_type::ok);
        connection->set_headers(json_headers);
        connection->write(json_string({
                    {"configured", kernel.is_configured()},
                    {"version", kernel.version}
                }));
    }

    void
    handle_configure(action_params const &params,
                     request_type const &request,
                     connection_ptr_type connection)
    {
        kernel.configure(request.body);

        connection->set_status(connection_type::ok);
        connection->set_headers(json_headers);
        connection->write(json_string({}));
    }

    void
    handle_enumerate(action_params const &params,
                     request_type const &request,
                     connection_ptr_type connection)
    {
        if (!kernel.is_configured()) {
            throw std::runtime_error("not configured");
        }

        kernel.enumeration_executor.add(
            [=] {
                try {
                    auto devices = kernel.enumerate_devices();

                    Json::Value no_session;
                    Json::Value item(Json::objectValue);
                    Json::Value list(Json::arrayValue);

                    for (auto const &device: devices) {
                        item["path"] = device.first;
                        item["session"] =
                            device.second.empty() ? no_session : device.second;
                        list.append(item);
                    }

                    connection->set_status(connection_type::ok);
                    connection->set_headers(json_headers);
                    connection->write(json_string(list));
                }
                catch (std::exception const &e) {
                    connection->set_status(connection_type::internal_server_error);
                    connection->set_headers(json_headers);
                    connection->write(json_string({{"error", e.what()}}));
                }
            });
    }

    void
    handle_acquire(action_params const &params,
                   request_type const &request,
                   connection_ptr_type connection)
    {
        if (!kernel.is_configured()) {
            throw std::runtime_error("not configured");
        }

        auto device_path = params.str(1);

        auto device = kernel.get_device_kernel(device_path);
        auto executor = kernel.get_device_executor(device_path);

        executor->add(
            [=] {
                try {
                    device->open();
                    auto session_id = kernel.acquire_session(device_path);
                    connection->set_status(connection_type::ok);
                    connection->set_headers(json_headers);
                    connection->write(json_string({{"session", session_id}}));
                }
                catch (std::exception const &e) {
                    connection->set_status(connection_type::internal_server_error);
                    connection->set_headers(json_headers);
                    connection->write(json_string({{"error", e.what()}}));
                }
            });
    }

    void
    handle_release(action_params const &params,
                   request_type const &request,
                   connection_ptr_type connection)
    {
        if (!kernel.is_configured()) {
            throw std::runtime_error("not configured");
        }

        auto session_id = params.str(1);

        auto session_it = kernel.find_session_by_id(session_id);
        if (session_it == kernel.sessions.end()) { // TODO: race condition
            connection->set_status(connection_type::not_found);
            connection->set_headers(json_headers);
            connection->write(json_string({{"error", "session not found"}}));
            return;
        }
        auto &device_path = session_it->first;

        auto device = kernel.get_device_kernel(device_path);
        auto executor = kernel.get_device_executor(device_path);

        executor->add(
            [=] {
                try {
                    device->close();
                    kernel.release_session(session_id);
                    connection->set_status(connection_type::ok);
                    connection->set_headers(json_headers);
                    connection->write(json_string({}));
                }
                catch (std::exception const &e) {
                    connection->set_status(connection_type::internal_server_error);
                    connection->set_headers(json_headers);
                    connection->write(json_string({{"error", e.what()}}));
                }
            });
    }

    void
    handle_call(action_params const &params,
                request_type const &request,
                connection_ptr_type connection)
    {
        if (!kernel.is_configured()) {
            throw std::runtime_error("not configured");
        }

        auto session_id = params.str(1);

        auto session_it = kernel.find_session_by_id(session_id);
        if (session_it == kernel.sessions.end()) { // TODO: race condition
            connection->set_status(connection_type::not_found);
            connection->set_headers(json_headers);
            connection->write(json_string({{"error", "session not found"}}));
            return;
        }
        auto &device_path = session_it->first;

        auto device = kernel.get_device_kernel(device_path);
        auto executor = kernel.get_device_executor(device_path);

        executor->add(
            [=] {
                try {
                    wire::message wire_in;
                    wire::message wire_out;

                    Json::Value json;
                    Json::Reader json_reader;
                    json_reader.parse(request.body, json);

                    json_to_wire(json, wire_in);
                    device->call(wire_in, wire_out);
                    wire_to_json(wire_out, json);

                    connection->set_status(connection_type::ok);
                    connection->set_headers(json_headers);
                    connection->write(json_string(json));
                }
                catch (std::exception const &e) {
                    connection->set_status(connection_type::internal_server_error);
                    connection->set_headers(json_headers);
                    connection->write(json_string({{"error", e.what()}}));
                }
            });
    }

    void
    handle_404(action_params const &params,
               request_type const &request,
               connection_ptr_type connection)
    {
        connection->set_status(connection_type::not_found);
        connection->set_headers(json_headers);
        connection->write(json_string({}));
    }

    // helper codec for converting between wire messages and json

    typedef std::unique_ptr< protobuf::pb::Message > protobuf_ptr;

    void
    json_to_wire(Json::Value const &json, wire::message &wire)
    {
        protobuf_ptr pbuf(kernel.pb_json_codec.typed_json_to_protobuf(json));
        kernel.pb_wire_codec.protobuf_to_wire(*pbuf, wire);
    }

    void
    wire_to_json(wire::message const &wire, Json::Value &json)
    {
        protobuf_ptr pbuf(kernel.pb_wire_codec.wire_to_protobuf(wire));
        json = kernel.pb_json_codec.protobuf_to_typed_json(*pbuf);
    }
};

template <typename Server, typename Handler>
struct body_reading_handler
    : boost::enable_shared_from_this<
          body_reading_handler<Server, Handler>
          >
{
    typedef Server server_type;
    typedef Handler handler_type;

    typedef typename server_type::request request_type;
    typedef typename server_type::connection connection_type;
    typedef typename server_type::connection_ptr connection_ptr_type;

    body_reading_handler(body_reading_handler const &) = delete;
    body_reading_handler &operator=(body_reading_handler const&) = delete;

    body_reading_handler(handler_type &handler_)
        : handler(handler_),
          body()
    {}

    void operator()(request_type const &request,
                    connection_ptr_type connection)
    {
        body_size = 0;
        body_max_size = read_content_length(request);
        read_body(request, connection);
    }

private:

    handler_type &handler;
    std::stringstream body;
    std::size_t body_size;
    std::size_t body_max_size;

    void
    read_body(request_type const &request,
              connection_ptr_type connection)
    {
        connection->read(
            boost::bind(
                &body_reading_handler<Server, Handler>::handle_body_read,
                body_reading_handler<Server, Handler>::shared_from_this(),
                _1, _2, _3, _4, request));
    }

    void
    handle_body_read(typename connection_type::input_range range,
                     boost::system::error_code error,
                     std::size_t size,
                     connection_ptr_type connection,
                     request_type const &request)
    {
        body.write(range.begin(), size);
        body_size += size;

        if (body_size < body_max_size) {
            read_body(request, connection);
        } else {
            request.body = body.str();
            handler(request, connection);
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

struct connection_handler
{
    typedef boost::network::http::async_server<
        connection_handler
        > server;
    typedef request_handler<server> handler_type;

    core::kernel kernel;
    handler_type handler;

    connection_handler()
        : kernel(),
          handler(kernel)
    {}

    void
    operator()(server::request const &request,
               server::connection_ptr connection)
    {
        auto br_handler = boost::make_shared<
            body_reading_handler<server, handler_type>
            >(handler);
        (*br_handler)(request, connection);
    }
};

}
}
