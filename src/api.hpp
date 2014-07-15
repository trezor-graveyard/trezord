#include "protobuf/json_codec.hpp"
#include "protobuf/wire_codec.hpp"

#include <boost/network/include/http/server.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <functional>
#include <iostream>
#include <string>
#include <regex>
#include <map>

namespace trezord
{
namespace api
{

// loosely based on:
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3562.pdf
struct async_executor
{
    async_executor()
        : io_service(),
          io_work(io_service),
          // cannot use std::bind here, see:
          // https://stackoverflow.com/questions/9048119
          thread(boost::bind(&io_service_type::run, &io_service))
    {}

    ~async_executor()
    {
        io_service.stop();
        thread.join();
    }

    void
    add(std::function<void()> const &closure)
    { io_service.post(closure); }

private:

    typedef boost::asio::io_service io_service_type;

    io_service_type io_service;
    io_service_type::work io_work;
    std::thread thread;
};

struct device_kernel
{
    typedef std::string device_path_type;

    device_path_type device_path;

    device_kernel(device_path_type const &dp)
        : device_path(dp)
    {}

    void
    open()
    {
        auto ptr = new wire::device(device_path);
        device.reset(ptr);
    }

    void
    close()
    { device.reset(); }

    void
    call(wire::message const &msg_in,
         wire::message &msg_out)
    {
        if (!device.get()) {
            open();
        }
        msg_in.write_to(*device);
        msg_out.read_from(*device);
    }

private:

    typedef std::unique_ptr<
        wire::device
        > wire_device_ptr_type;

    wire_device_ptr_type device;
};

struct app_kernel
{
    typedef std::string session_id_type;
    typedef std::string device_path_type;

    std::mutex mutex;

    std::map<
        device_path_type,
        session_id_type
        > sessions;

    std::map<
        device_path_type,
        device_kernel
        > device_kernels;

    std::map<
        device_path_type,
        async_executor
        > device_executors;

    async_executor enumeration_executor;

    protobuf::state pb_state;
    protobuf::wire_codec pb_wire_codec;
    protobuf::json_codec pb_json_codec;

    app_kernel()
        : sessions(),
          device_kernels(),
          device_executors(),
          enumeration_executor(),
          pb_state(),
          pb_wire_codec(pb_state),
          pb_json_codec(pb_state)
    {}

    device_kernel *
    get_device_kernel(device_path_type const &device_path)
    {
        std::unique_lock<std::mutex> lock(mutex);
        auto kernel_r = device_kernels.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(device_path),
            std::forward_as_tuple(device_path));
        return &kernel_r.first->second;
    }

    async_executor *
    get_device_executor(device_path_type const &device_path)
    {
        std::unique_lock<std::mutex> lock(mutex);
        auto executor_r = device_executors.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(device_path),
            std::forward_as_tuple());
        return &executor_r.first->second;
    }

    session_id_type
    acquire_session(device_path_type const &device_path)
    {
        std::unique_lock<std::mutex> lock(mutex);
        auto session_id = generate_session_id();
        sessions[device_path] = session_id;
        return session_id;
    }

    void
    release_session(session_id_type const &session_id)
    {
        std::unique_lock<std::mutex> lock(mutex);
        auto session_it = find_session_by_id(session_id);
        if (session_it != sessions.end()) {
            sessions.erase(session_it);
        }
    }

    decltype(sessions)::iterator
    find_session_by_id(session_id_type const &session_id)
    {
        std::unique_lock<std::mutex> lock(mutex);
        return std::find_if(
            sessions.begin(),
            sessions.end(),
            [&] (decltype(sessions)::value_type const &kv) {
                return kv.second == session_id;
            });
    }

private:

    boost::uuids::random_generator uuid_generator;

    session_id_type
    generate_session_id()
    {
        return boost::lexical_cast<
            session_id_type
            >(uuid_generator());
    }
};

struct handler
{
    typedef boost::network::http::async_server<handler> server;

    void
    operator()(server::request const &request,
               server::connection_ptr connection)
    {
        connection->read(
            [&] (boost::iterator_range<char const *> input_range,
                 boost::system::error_code error_code,
                 std::size_t size,
                 server::connection_ptr connection)
            {
                try {
                    request.body = boost::copy_range<std::string>(input_range);
                    action_handler ahandler;
                    action_params params;

                    dispatch(request, ahandler, params);
                    ahandler(this, params, request, connection);
                }
                catch (std::exception const &e) {
                    connection->set_status(server::connection::internal_server_error);
                    connection->set_headers(json_headers);
                    connection->write(json_string({{"error", e.what()}}));
                }
            });
    }

private:

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

    app_kernel k;

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

        for (auto &kv: list) {
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
        for (auto &route: action_routes) {
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
                    {"configured", false},
                    {"version", "0.0.1"}
                }));
    }

    void
    handle_enumerate(action_params const &params,
                     server::request const &request,
                     server::connection_ptr connection)
    {
        k.enumeration_executor.add(
            [=] {
                auto device_paths = wire::enumerate_devices(
                    [&] (hid_device_info const *i) {
                        return i->vendor_id == 0x534c &&
                               i->product_id == 0x0001;
                    });

                Json::Value list(Json::arrayValue);
                for (auto &path: device_paths) {
                    list.append(path);
                }

                connection->set_status(server::connection::ok);
                connection->set_headers(json_headers);
                connection->write(json_string(list));
            });
    }

    void
    handle_configure(action_params const &params,
                     server::request const &request,
                     server::connection_ptr connection)
    {
        // TODO
    }

    void
    handle_acquire(action_params const &params,
                   server::request const &request,
                   server::connection_ptr connection)
    {
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
