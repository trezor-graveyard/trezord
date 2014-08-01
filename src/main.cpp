#ifdef _WIN32
#  define _ELPP_DEFAULT_LOG_FILE "C:\\Windows\\Temp\\trezord.log"
#else
#  define _ELPP_DEFAULT_LOG_FILE "/var/log/trezord.log"
#endif

#include <easylogging++.h>

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "utils.hpp"
#include "wire.hpp"
#include "core.hpp"
#include "api.hpp"

_INITIALIZE_EASYLOGGINGPP

static const auto server_port = "21324";
static const auto server_address = "127.0.0.1";
static const auto sleep_time = boost::posix_time::seconds(10);

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

void
shutdown_server(boost::system::error_code const &error,
                int signal,
                trezord::api::connection_handler::server &server)
{
    if (!error) {
        LOG(INFO) << "signal: " << signal << ", stopping server";
        server.stop();
    }
}

void
start_server()
{
    using namespace boost;
    using namespace trezord;

    typedef api::connection_handler::server server_type;

    core::kernel kernel;

    // http handlers
    api::request_handler<server_type> request_handler(kernel);
    api::connection_handler connection_handler(request_handler);

    // thread group
    auto threads = make_shared<thread_group>();

    // io service
    auto io_service = make_shared<asio::io_service>();
    auto io_work = make_shared<asio::io_service::work>(ref(*io_service));
    threads->create_thread(bind(&asio::io_service::run, io_service));
    threads->create_thread(bind(&asio::io_service::run, io_service));
    threads->create_thread(bind(&asio::io_service::run, io_service));

    // thread pool
    auto thread_pool = make_shared<network::utils::thread_pool>(2);

    // http server
    server_type::options options(connection_handler);
    server_type server(
        options
        .reuse_address(true)
        .io_service(io_service)
        .thread_pool(thread_pool)
        .address(server_address)
        .port(server_port));

    // signal handling for clear shutdown
    asio::signal_set signals(ref(*io_service), SIGINT, SIGTERM);
    signals.async_wait(bind(shutdown_server, _1, _2, ref(server)));

    // start the server
    LOG(INFO) << "starting server";
    server.run();

    // server has finished now
    LOG(INFO) << "server finished running";
}

int
main(int argc, char *argv[])
{
    _START_EASYLOGGINGPP(argc, argv);
    configure_logging();

    bool restart_server = false;

    do {
        try {
            start_server();
            restart_server = false;
        }
        catch (std::exception const &e) {
            LOG(ERROR) << e.what();
            LOG(INFO) << "sleeping for " << sleep_time;
            boost::this_thread::sleep(sleep_time);
            restart_server = true;
        }
    }
    while (restart_server);

    return 0;
}
