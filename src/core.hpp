#include "protobuf/json_codec.hpp"
#include "protobuf/wire_codec.hpp"
#include "config/config.pb.h"
#include "crypto.hpp"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <boost/thread.hpp>

namespace trezord
{
namespace core
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
    boost::thread thread;
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
        if (device.get() == nullptr) {
            auto ptr = new wire::device(device_path.c_str());
            device.reset(ptr);
        }
    }

    void
    close()
    {
        device.reset();
    }

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

    std::unique_ptr< wire::device > device;
};

struct kernel
{
    typedef std::string session_id_type;
    typedef std::string device_path_type;
    typedef std::vector<
        std::pair<device_path_type, session_id_type>
        > device_enumeration_type;

    struct missing_configuration
        : public std::logic_error
    { using std::logic_error::logic_error; };

    struct invalid_configuration
        : public std::invalid_argument
    { using std::invalid_argument::invalid_argument; };

    struct invalid_session
        : public std::invalid_argument
    { using std::invalid_argument::invalid_argument; };

public:

    kernel()
        : sessions(),
          device_kernels(),
          device_executors(),
          enumeration_executor(),
          pb_state(),
          pb_wire_codec(pb_state),
          pb_json_codec(pb_state)
    {}

    std::string
    get_version()
    {
        return "0.0.1";
    }

    // configuration management

    void
    configure(std::string const &config_str)
    {
        lock_type lock(mutex);

        if (config_str.size() <= 64) {
            throw invalid_configuration("configuration string is malformed");
        }

        auto config_sig = (const std::uint8_t *)(config_str.data());
        auto config_msg = (const std::uint8_t *)(config_str.data()) + 64;
        auto config_msg_len = config_str.size() - 64;

        static const char *signature_keys[] = {
            #include "config/keys.h"
        };

        auto keys = (const std::uint8_t **)(signature_keys);
        auto keys_len = sizeof(signature_keys) / sizeof(signature_keys[0]);

        if (!crypto::verify_signature(
                config_sig, config_msg, config_msg_len, keys, keys_len)) {
            throw invalid_configuration("configuration signature is not correct");
        }

        configuration.ParseFromArray(config_msg, config_msg_len);
        pb_state.build_from_set(configuration.wire_protocol());
        pb_wire_codec.load_protobuf_state();
    }

    bool
    is_configured()
    {
        lock_type lock(mutex);
        return configuration.IsInitialized();
    }

    // device enumeration

    async_executor *
    get_enumeration_executor()
    {
        return &enumeration_executor;
    }

    device_enumeration_type
    enumerate_devices()
    {
        lock_type lock(mutex);

        require_configuration();

        auto devices = wire::enumerate_devices(
            [] (hid_device_info const *i) {
                return i->vendor_id == 0x534c && i->product_id == 0x0001;
            });

        device_enumeration_type device_list;
        for (auto const &path: devices) {
            auto it = sessions.find(path);
            if (it != sessions.end()) {
                device_list.emplace_back(path, it->second);
            } else {
                device_list.emplace_back(path, "");
            }
        }
        return device_list;
    }

    // device kernels and executors

    device_kernel *
    get_device_kernel(device_path_type const &device_path)
    {
        lock_type lock(mutex);

        require_configuration();

        auto kernel_r = device_kernels.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(device_path),
            std::forward_as_tuple(device_path));

        return &kernel_r.first->second;
    }

    device_kernel *
    get_device_kernel_by_session_id(session_id_type const &session_id)
    {
        lock_type lock(mutex);

        require_configuration();

        auto session_it = find_session_by_id(session_id);
        if (session_it == sessions.end()) {
            throw invalid_session("session not found");
        }

        return get_device_kernel(session_it->first);
    }

    async_executor *
    get_device_executor(device_path_type const &device_path)
    {
        lock_type lock(mutex);

        require_configuration();

        auto executor_r = device_executors.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(device_path),
            std::forward_as_tuple());

        return &executor_r.first->second;
    }

    async_executor *
    get_device_executor_by_session_id(session_id_type const &session_id)
    {
        lock_type lock(mutex);

        require_configuration();

        auto session_it = find_session_by_id(session_id);
        if (session_it == sessions.end()) {
            throw invalid_session("session not found");
        }

        return get_device_executor(session_it->first);
    }

    // session management

    session_id_type
    acquire_session(device_path_type const &device_path)
    {
        lock_type lock(mutex);

        require_configuration();

        return sessions[device_path] = generate_session_id();
    }

    void
    release_session(session_id_type const &session_id)
    {
        lock_type lock(mutex);

        require_configuration();

        auto session_it = find_session_by_id(session_id);
        if (session_it != sessions.end()) {
            sessions.erase(session_it);
        }
    }

    // protobuf <-> json codec

    typedef std::unique_ptr<protobuf::pb::Message> protobuf_ptr;

    void
    json_to_wire(Json::Value const &json, wire::message &wire)
    {
        protobuf_ptr pbuf(pb_json_codec.typed_json_to_protobuf(json));
        pb_wire_codec.protobuf_to_wire(*pbuf, wire);
    }

    void
    wire_to_json(wire::message const &wire, Json::Value &json)
    {
        protobuf_ptr pbuf(pb_wire_codec.wire_to_protobuf(wire));
        json = pb_json_codec.protobuf_to_typed_json(*pbuf);
    }

private:

    boost::recursive_mutex mutex;
    typedef boost::unique_lock<decltype(mutex)> lock_type;

    std::map<device_path_type, session_id_type> sessions;
    std::map<device_path_type, device_kernel> device_kernels;
    std::map<device_path_type, async_executor> device_executors;
    async_executor enumeration_executor;

    Configuration configuration;
    protobuf::state pb_state;
    protobuf::wire_codec pb_wire_codec;
    protobuf::json_codec pb_json_codec;
    boost::uuids::random_generator uuid_generator;

    void
    require_configuration()
    {
        if (!is_configured()) {
            throw missing_configuration("not configured");
        }
    }

    session_id_type
    generate_session_id()
    {
        return boost::lexical_cast<session_id_type>(uuid_generator());
    }

    decltype(sessions)::iterator
    find_session_by_id(session_id_type const &session_id)
    {
        return std::find_if(
            sessions.begin(),
            sessions.end(),
            [&] (decltype(sessions)::value_type const &kv) {
                return kv.second == session_id;
            });
    }
};

}
}
