#ifdef __linux__
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#define BOOST_NETWORK_ENABLE_HTTPS 1
#include <boost/network/include/http/client.hpp>

#define _ELPP_THREAD_SAFE 1
#define _ELPP_FORCE_USE_STD_THREAD 1
#define _ELPP_NO_DEFAULT_LOG_FILE

#include <easylogging++.h>

#include "utils.hpp"
#include "wire.hpp"
#include "core.hpp"
#include "api.hpp"

_INITIALIZE_EASYLOGGINGPP

static const auto server_port = "21324";
static const auto server_address = "127.0.0.1";

static const auto https_cert_uri = "https://mytrezor.com/data/bridge/cert/server.crt";
static const auto https_privkey_uri = "https://mytrezor.com/data/bridge/cert/server.key";

static const auto sleep_time = boost::posix_time::seconds(10);

std::string
get_log_path()
{
#ifdef _WIN32

    auto app_data = getenv("APPDATA");
    if (!app_data) {
        throw std::runtime_error{"env var APPDATA not found"};
    }
    return std::string{app_data} + "\\TREZOR Bridge\\trezord.log";

#endif
#ifdef __APPLE__

    auto home = getenv("HOME");
    if (!home) {
        throw std::runtime_error("env var HOME not found");
    }
    return std::string{home} + "/Library/Logs/trezord.log";

#endif

    return "/var/log/trezord.log";
}

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

    el::Configurations config;

    config.setToDefault();
    config.setGlobally(
        el::ConfigurationType::Filename,
        get_log_path());
    config.setGlobally(
        el::ConfigurationType::Format,
        "%datetime %level [%logger] [%thread] %msg");
    config.set(
        el::Level::Debug,
        el::ConfigurationType::Format,
        "%datetime %level [%logger] [%thread] %msg");
    config.set(
        el::Level::Trace,
        el::ConfigurationType::Format,
        "%datetime %level [%logger] [%thread] %msg");
    config.set(
        el::Level::Verbose,
        el::ConfigurationType::Format,
        "%datetime %level-%vlevel [%logger] [%thread] %msg");

    el::Loggers::reconfigureAllLoggers(config);
}

template <typename Server>
void
shutdown_server(boost::system::error_code const &error,
                int signal,
                Server &server)
{
    if (!error) {
        LOG(INFO) << "signal: " << signal << ", stopping server";
        server.stop();
    }
}

std::string
download_uri(const std::string &uri)
{
    using namespace boost::network;

    http::client::options options;
    http::client client{options.follow_redirects(true)};
    http::client::request request{uri};

    LOG(INFO) << "requesting " << uri;
    http::client::response response = client.get(request);
    LOG(INFO) << "response " << int(status(response));

    if (status(response) != 200) {
        throw std::runtime_error{"request failed"};
    }
    return body(response);
}

void
configure_https(boost::asio::ssl::context &context)
{
    context.set_options(
        boost::asio::ssl::context::default_workarounds
        | boost::asio::ssl::context::no_sslv2
        | boost::asio::ssl::context::single_dh_use);

    trezord::crypto::ssl::load_privkey(
        context.native_handle(),
        download_uri(https_privkey_uri));

    trezord::crypto::ssl::load_cert(
        context.native_handle(),
        download_uri(https_cert_uri));
}

void
start_server()
{
    using namespace boost;
    using namespace trezord;

    using server_type = api::connection_handler::server;

    core::kernel kernel;

    // http handlers
    api::request_handler<server_type> request_handler{kernel};
    api::connection_handler connection_handler{request_handler};

    // thread group
    auto threads = make_shared<thread_group>();

    // io service
    auto io_service = make_shared<asio::io_service>();
    auto io_work = make_shared<asio::io_service::work>(boost::ref(*io_service));
    threads->create_thread(bind(&asio::io_service::run, io_service));
    threads->create_thread(bind(&asio::io_service::run, io_service));
    threads->create_thread(bind(&asio::io_service::run, io_service));

    // thread pool
    auto thread_pool = make_shared<network::utils::thread_pool>(2);

    // https
    boost::asio::ssl::context context{boost::asio::ssl::context::sslv23};
    configure_https(context);

    // server
    server_type::options options{connection_handler};
    server_type server{
        options
        .reuse_address(true)
        .io_service(io_service)
        .thread_pool(thread_pool)
        .address(server_address)
        .port(server_port),
        context};

    // signal handling for clear shutdown
    asio::signal_set signals{boost::ref(*io_service), SIGINT, SIGTERM};
    signals.async_wait(
        bind(&shutdown_server<server_type>, _1, _2, boost::ref(server)));

    // start the server
    LOG(INFO) << "starting server";
    server.run();

    // server has finished now
    LOG(INFO) << "server finished running";
}

#ifdef __linux__
void
daemonize()
{
    // first fork
    if (pid_t pid = fork()) {
        if (pid > 0) {
            exit(0);
        } else {
            LOG(ERROR) << "first fork failed";
            exit(1);
        }
    }
    setsid();
    chdir("/");
    umask(0);
    // second fork
    if (pid_t pid = fork()) {
        if (pid > 0) {
            exit(0);
        } else {
            LOG(ERROR) << "first fork failed";
            exit(1);
        }
    }
    close(0);
    close(1);
    close(2);
    if (open("/dev/null", O_RDONLY) < 0) {
        LOG(ERROR) << "unable to open /dev/null";
        exit(1);
    }
}
#endif

int
main(int argc, char *argv[])
{
    _START_EASYLOGGINGPP(argc, argv);
    configure_logging();

#ifdef __linux__
    daemonize();
#endif

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
