#ifdef _WIN32
#  define _ELPP_DEFAULT_LOG_FILE "logs\\trezord.log"
#else
#  define _ELPP_DEFAULT_LOG_FILE "logs/trezord.log"
#endif

#include <easylogging++.h>

#include "utils.hpp"
#include "wire.hpp"
#include "core.hpp"
#include "api.hpp"

_INITIALIZE_EASYLOGGINGPP

void
configure_logging()
{
    el::Loggers::getLogger("http");
    el::Loggers::getLogger("http.body");
    el::Loggers::getLogger("http.cors");

    el::Loggers::getLogger("core.device");
    el::Loggers::getLogger("core.config");
    el::Loggers::getLogger("core.kernel");

    el::Loggers::getLogger("wire.enumerate");

    // easylogging has a %thread format with std::thread support,
    // sadly we need to use boost::thread to accomodate gcc 4.8
    el::Helpers::installCustomFormatSpecifier(
        el::CustomFormatSpecifier("%curr_thread", [] {
                return boost::lexical_cast<std::string>(
                    boost::this_thread::get_id());
            }));

    el::Configurations config;

    config.setToDefault();
    config.setGlobally(
        el::ConfigurationType::Format,
        "%datetime %level [%logger] [%curr_thread] %msg");
    config.set(
        el::Level::Debug,
        el::ConfigurationType::Format,
        "%datetime %level [%logger] [%curr_thread] %msg");
    config.set(
        el::Level::Trace,
        el::ConfigurationType::Format,
        "%datetime %level [%logger] [%curr_thread] %msg");
    config.set(
        el::Level::Verbose,
        el::ConfigurationType::Format,
        "%datetime %level-%vlevel [%logger] [%curr_thread] %msg");

    el::Loggers::reconfigureAllLoggers(config);
}

int
main(int argc, char *argv[])
{
    _START_EASYLOGGINGPP(argc, argv);

    configure_logging();

    using namespace trezord;

    core::kernel kernel;
    api::request_handler<
        api::connection_handler::server> request_handler(kernel);
    api::connection_handler connection_handler(request_handler);

    auto thread_pool = boost::make_shared<
        boost::network::utils::thread_pool>(2);
    api::connection_handler::server::options options(connection_handler);
    api::connection_handler::server server(
        options
        .thread_pool(thread_pool)
        .reuse_address(true)
        .address("127.0.0.1")
        .port("21324"));

    LOG(INFO) << "starting server";
    server.run();
    LOG(INFO) << "server finished running";

    return 0;
}
