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

static const char *signature_keys[] = {
    // sample.key "correct horse battery staple"
    // "\x04\x78\xd4\x30\x27\x4f\x8c\x5e\xc1\x32\x13\x38\x15\x1e\x9f\x27\xf4\xc6\x76\xa0\x08\xbd\xf8\x63\x8d\x07\xc0\xb6\xbe\x9a\xb3\x5c\x71\xa1\x51\x80\x63\x24\x3a\xcd\x4d\xfe\x96\xb6\x6e\x3f\x2e\xc8\x01\x3c\x8e\x07\x2c\xd0\x9b\x38\x34\xa1\x9f\x81\xf6\x59\xcc\x34\x55"
    // production keys
    "\x04\xd5\x71\xb7\xf1\x48\xc5\xe4\x23\x2c\x38\x14\xf7\x77\xd8\xfa\xea\xf1\xa8\x42\x16\xc7\x8d\x56\x9b\x71\x04\x1f\xfc\x76\x8a\x5b\x2d\x81\x0f\xc3\xbb\x13\x4d\xd0\x26\xb5\x7e\x65\x00\x52\x75\xae\xde\xf4\x3e\x15\x5f\x48\xfc\x11\xa3\x2e\xc7\x90\xa9\x33\x12\xbd\x58",
    "\x04\x63\x27\x9c\x0c\x08\x66\xe5\x0c\x05\xc7\x99\xd3\x2b\xd6\xba\xb0\x18\x8b\x6d\xe0\x65\x36\xd1\x10\x9d\x2e\xd9\xce\x76\xcb\x33\x5c\x49\x0e\x55\xae\xe1\x0c\xc9\x01\x21\x51\x32\xe8\x53\x09\x7d\x54\x32\xed\xa0\x6b\x79\x20\x73\xbd\x77\x40\xc9\x4c\xe4\x51\x6c\xb1",
    "\x04\x43\xae\xdb\xb6\xf7\xe7\x1c\x56\x3f\x8e\xd2\xef\x64\xec\x99\x81\x48\x25\x19\xe7\xef\x4f\x4a\xa9\x8b\x27\x85\x4e\x8c\x49\x12\x6d\x49\x56\xd3\x00\xab\x45\xfd\xc3\x4c\xd2\x6b\xc8\x71\x0d\xe0\xa3\x1d\xbd\xf6\xde\x74\x35\xfd\x0b\x49\x2b\xe7\x0a\xc7\x5f\xde\x58",
    "\x04\x87\x7c\x39\xfd\x7c\x62\x23\x7e\x03\x82\x35\xe9\xc0\x75\xda\xb2\x61\x63\x0f\x78\xee\xb8\xed\xb9\x24\x87\x15\x9f\xff\xed\xfd\xf6\x04\x6c\x6f\x8b\x88\x1f\xa4\x07\xc4\xa4\xce\x6c\x28\xde\x0b\x19\xc1\xf4\xe2\x9f\x1f\xcb\xc5\xa5\x8f\xfd\x14\x32\xa3\xe0\x93\x8a",
    "\x04\x73\x84\xc5\x1a\xe8\x1a\xdd\x0a\x52\x3a\xdb\xb1\x86\xc9\x1b\x90\x6f\xfb\x64\xc2\xc7\x65\x80\x2b\xf2\x6d\xbd\x13\xbd\xf1\x2c\x31\x9e\x80\xc2\x21\x3a\x13\x6c\x8e\xe0\x3d\x78\x74\xfd\x22\xb7\x0d\x68\xe7\xde\xe4\x69\xde\xcf\xbb\xb5\x10\xee\x9a\x46\x0c\xda\x45",
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

    boost::mutex mutex;

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
        boost::unique_lock<boost::mutex> lock(mutex);
        configuration.ParseFromString(config_str);
    }

    bool
    is_configured()
    {
        boost::unique_lock<boost::mutex> lock(mutex);
        return configuration.IsInitialized();
    }

    device_kernel *
    get_device_kernel(device_path_type const &device_path)
    {
        boost::unique_lock<boost::mutex> lock(mutex);
        auto kernel_r = device_kernels.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(device_path),
            std::forward_as_tuple(device_path));
        return &kernel_r.first->second;
    }

    async_executor *
    get_device_executor(device_path_type const &device_path)
    {
        boost::unique_lock<boost::mutex> lock(mutex);
        auto executor_r = device_executors.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(device_path),
            std::forward_as_tuple());
        return &executor_r.first->second;
    }

    session_id_type
    acquire_session(device_path_type const &device_path)
    {
        boost::unique_lock<boost::mutex> lock(mutex);
        auto session_id = generate_session_id();
        sessions[device_path] = session_id;
        return session_id;
    }

    void
    release_session(session_id_type const &session_id)
    {
        boost::unique_lock<boost::mutex> lock(mutex);
        auto session_it = find_session_by_id(session_id);
        if (session_it != sessions.end()) {
            sessions.erase(session_it);
        }
    }

    decltype(sessions)::iterator
                       find_session_by_id(session_id_type const &session_id)
    {
        boost::unique_lock<boost::mutex> lock(mutex);
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
