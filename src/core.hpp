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

#include <thread>
#include <mutex>

namespace trezord
{
namespace core
{

static const char *signature_keys[] = {
    // sample.key
    //    "-----BEGIN PUBLIC KEY-----\n"
    //    "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAENEoHRLyHw1AM5jl0pbUNgXHFldzn1jWT\n"
    //    "/tGrlePY/vfKVVc2QM2pz0fbYUn4WEZC8DDfQ3XEhvKnsyOotsY7QQ==\n"
    //    "-----END PUBLIC KEY-----\n",

    // production keys
    "-----BEGIN PUBLIC KEY-----\n"
    "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAE1XG38UjF5CMsOBT3d9j66vGoQhbHjVab\n"
    "cQQf/HaKWy2BD8O7E03QJrV+ZQBSda7e9D4VX0j8EaMux5CpMxK9WA==\n"
    "-----END PUBLIC KEY-----\n",
    "-----BEGIN PUBLIC KEY-----\n"
    "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEYyecDAhm5QwFx5nTK9a6sBiLbeBlNtEQ\n"
    "nS7ZznbLM1xJDlWu4QzJASFRMuhTCX1UMu2ga3kgc713QMlM5FFssQ==\n"
    "-----END PUBLIC KEY-----\n",
    "-----BEGIN PUBLIC KEY-----\n"
    "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEQ67btvfnHFY/jtLvZOyZgUglGefvT0qp\n"
    "iyeFToxJEm1JVtMAq0X9w0zSa8hxDeCjHb323nQ1/QtJK+cKx1/eWA==\n"
    "-----END PUBLIC KEY-----\n",
    "-----BEGIN PUBLIC KEY-----\n"
    "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEh3w5/XxiI34DgjXpwHXasmFjD3juuO25\n"
    "JIcVn//t/fYEbG+LiB+kB8Skzmwo3gsZwfTinx/LxaWP/RQyo+CTig==\n"
    "-----END PUBLIC KEY-----\n",
    "-----BEGIN PUBLIC KEY-----\n"
    "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEc4TFGuga3QpSOtuxhskbkG/7ZMLHZYAr\n"
    "8m29E73xLDGegMIhOhNsjuA9eHT9IrcNaOfe5Gnez7u1EO6aRgzaRQ==\n"
    "-----END PUBLIC KEY-----\n"
};

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

struct kernel
{
    typedef std::string session_id_type;
    typedef std::string device_path_type;

    const std::string version = "0.0.1";

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

    Configuration configuration;

    protobuf::state pb_state;
    protobuf::wire_codec pb_wire_codec;
    protobuf::json_codec pb_json_codec;

    kernel()
        : sessions(),
          device_kernels(),
          device_executors(),
          enumeration_executor(),
          pb_state(),
          pb_wire_codec(pb_state),
          pb_json_codec(pb_state)
    {}

    void
    configure(std::string const &config_str)
    {
        std::unique_lock<std::mutex> lock(mutex);
        configuration.ParseFromString(config_str);
    }

    bool
    is_configured()
    {
        std::unique_lock<std::mutex> lock(mutex);
        return configuration.IsInitialized();
    }

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
        return boost::lexical_cast<session_id_type>(uuid_generator());
    }
};

}
}
