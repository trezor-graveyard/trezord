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

struct handler
{
    typedef boost::network::http::async_server<handler> server;

    struct connection_reader
    {
        std::size_t stream_size;
        std::stringstream stream;

        connection_reader(std::size_t max_size_,
                          server::connection_ptr connection_,
                          std::function<void(connection_reader *)> callback_)
            : stream_size(0),
              stream(),
              max_size(max_size_),
              connection(connection_),
              callback(callback_)
        {}

        void
        read()
        {
            connection->read(
                [&] (boost::iterator_range<char const *> input_range,
                     boost::system::error_code error_code,
                     std::size_t size,
                     server::connection_ptr connection_)
                {
                    stream.write(input_range.begin(), size);
                    stream_size += size;

                    if (stream_size < max_size) {
                        read();
                    } else {
                        callback(this);
                    }
                });
        }

    private:

        std::size_t max_size;
        server::connection_ptr connection;
        std::function<void(connection_reader *)> callback;
    };

    void
    operator()(server::request const &request,
               server::connection_ptr connection)
    {
        connection_reader *reader = new connection_reader(
            read_content_length(request),
            connection,
            [=] (connection_reader *reader_)
            {
                try {
                    action_handler ahandler;
                    action_params params;

                    request.body = reader_->stream.str();
                    delete reader_;

                    dispatch(request, ahandler, params);
                    ahandler(this, params, request, connection);
                }
                catch (std::exception const &e) {
                    try {
                        connection->set_status(server::connection::internal_server_error);
                        connection->set_headers(json_headers);
                        connection->write(json_string({{"error", e.what()}}));
                    } catch (...) {
                        // do nothing, just dont crash the server please
                    }
                }
            });

        reader->read();
    }

private:

    std::size_t
    read_content_length(server::request const &request)
    {
        auto content_length_header_it = std::find_if(
            request.headers.begin(),
            request.headers.end(),
            [&] (server::request::header_type const &header) {
                return header.name == "Content-Length";
            });
        if (content_length_header_it == request.headers.end()) {
            return 0;
        }

        return boost::lexical_cast<
            std::size_t
        >(content_length_header_it->value);
    }

    typedef std::smatch action_params;
    typedef std::function<
        void (handler*,
              action_params const &,
              server::request const &,
              server::connection_ptr)
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
        { "GET",  "/",             &handler::handle_index },
        { "GET",  "/enumerate",    &handler::handle_enumerate },
        { "POST", "/configure",    &handler::handle_configure },
        { "POST", "/acquire/(.+)", &handler::handle_acquire },
        { "POST", "/release/(.+)", &handler::handle_release },
        { "POST", "/call/(.+)",    &handler::handle_call },
        { ".*",   ".*",            &handler::handle_404 },
    };

    core::kernel k;

    // json support

    typedef std::pair<
        std::string,
        Json::Value
        > json_pair;

    typedef std::initializer_list<
        json_pair
        > json_list;

    const std::array<
        server::response_header, 1
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

    void
    dispatch(server::request const &request,
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
                 server::request const &request,
                 server::connection_ptr connection)
    {
        connection->set_status(server::connection::ok);
        connection->set_headers(json_headers);
        connection->write(json_string({
                    {"configured", k.is_configured()},
                    {"version", k.version}
                }));
    }

    void
    handle_configure(action_params const &params,
                     server::request const &request,
                     server::connection_ptr connection)
    {
        k.configure(request.body);

        connection->set_status(server::connection::ok);
        connection->set_headers(json_headers);
        connection->write(json_string({}));
    }

    void
    handle_enumerate(action_params const &params,
                     server::request const &request,
                     server::connection_ptr connection)
    {
        if (!k.is_configured()) {
            throw std::runtime_error("not configured");
        }

        k.enumeration_executor.add(
            [=] {
                try {
                    auto device_paths = wire::enumerate_devices(
                        [&] (hid_device_info const *i) {
                            return i->vendor_id == 0x534c &&
                            i->product_id == 0x0001;
                        });

                    Json::Value list(Json::arrayValue);
                    for (auto const &path: device_paths) {
                        list.append(path);
                    }

                    connection->set_status(server::connection::ok);
                    connection->set_headers(json_headers);
                    connection->write(json_string(list));
                }
                catch (std::exception const &e) {
                    connection->set_status(server::connection::internal_server_error);
                    connection->set_headers(json_headers);
                    connection->write(json_string({{"error", e.what()}}));
                }
            });
    }

    void
    handle_acquire(action_params const &params,
                   server::request const &request,
                   server::connection_ptr connection)
    {
        if (!k.is_configured()) {
            throw std::runtime_error("not configured");
        }

        auto device_path = params.str(1);

        auto kernel = k.get_device_kernel(device_path);
        auto executor = k.get_device_executor(device_path);

        executor->add(
            [=] {
                try {
                    kernel->open();
                    auto session_id = k.acquire_session(device_path);
                    connection->set_status(server::connection::ok);
                    connection->set_headers(json_headers);
                    connection->write(json_string({{"session", session_id}}));
                }
                catch (std::exception const &e) {
                    connection->set_status(server::connection::internal_server_error);
                    connection->set_headers(json_headers);
                    connection->write(json_string({{"error", e.what()}}));
                }
            });
    }

    void
    handle_release(action_params const &params,
                   server::request const &request,
                   server::connection_ptr connection)
    {
        if (!k.is_configured()) {
            throw std::runtime_error("not configured");
        }

        auto session_id = params.str(1);

        auto session_it = k.find_session_by_id(session_id);
        if (session_it == k.sessions.end()) { // TODO: race condition
            connection->set_status(server::connection::not_found);
            connection->write(json_string({{"error", "session not found"}}));
            return;
        }
        auto &device_path = session_it->first;

        auto kernel = k.get_device_kernel(device_path);
        auto executor = k.get_device_executor(device_path);

        executor->add(
            [=] {
                try {
                    kernel->close();
                    k.release_session(session_id);
                    connection->set_status(server::connection::ok);
                    connection->set_headers(json_headers);
                    connection->write(json_string({}));
                }
                catch (std::exception const &e) {
                    connection->set_status(server::connection::internal_server_error);
                    connection->set_headers(json_headers);
                    connection->write(json_string({{"error", e.what()}}));
                }
            }
        );
    }

    void
    handle_call(action_params const &params,
                server::request const &request,
                server::connection_ptr connection)
    {
        if (!k.is_configured()) {
            throw std::runtime_error("not configured");
        }

        auto session_id = params.str(1);

        auto session_it = k.find_session_by_id(session_id);
        if (session_it == k.sessions.end()) {
            connection->set_status(server::connection::not_found);
            connection->write(json_string({{"error", "session not found"}}));
            return;
        }
        auto &device_path = session_it->first;

        auto kernel = k.get_device_kernel(device_path);
        auto executor = k.get_device_executor(device_path);

        executor->add(
            [=] {
                try {
                    wire::message wire_in;
                    wire::message wire_out;

                    Json::Value json;
                    Json::Reader json_reader;
                    json_reader.parse(request.body, json);

                    json_to_wire(json, wire_in);
                    kernel->call(wire_in, wire_out);
                    wire_to_json(wire_out, json);

                    connection->set_status(server::connection::ok);
                    connection->write(json_string(json));
                }
                catch (std::exception const &e) {
                    connection->set_status(server::connection::internal_server_error);
                    connection->set_headers(json_headers);
                    connection->write(json_string({{"error", e.what()}}));
                }
            });
    }

    void
    handle_404(action_params const &params,
               server::request const &request,
               server::connection_ptr connection)
    {
        connection->set_status(server::connection::not_found);
        connection->set_headers(json_headers);
        connection->write(json_string({}));
    }

    // helper codec for converting between wire messages and json

    typedef std::unique_ptr<
        protobuf::pb::Message
        > protobuf_ptr;

    void
    json_to_wire(Json::Value const &json,
                 wire::message &wire)
    {
        protobuf_ptr pbuf(k.pb_json_codec.typed_json_to_protobuf(json));
        k.pb_wire_codec.protobuf_to_wire(*pbuf, wire);
    }

    void
    wire_to_json(wire::message const &wire,
                 Json::Value &json)
    {
        protobuf_ptr pbuf(k.pb_wire_codec.wire_to_protobuf(wire));
        json = k.pb_json_codec.protobuf_to_typed_json(*pbuf);
    }
};

}
}
