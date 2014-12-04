/*
 * This file is part of the TREZOR project.
 *
 * Copyright (C) 2014 SatoshiLabs
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef __linux__
#include <unistd.h>
#endif

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>

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
        el::ConfigurationType::MaxLogFileSize,
        "2097152"); // 2 MB
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
    auto privkey = download_uri(https_privkey_uri);
    auto cert_chain = download_uri(https_cert_uri);

    context.set_options(boost::asio::ssl::context::default_workarounds);
    context.set_verify_mode(boost::asio::ssl::verify_none);
    context.use_private_key(
        boost::asio::buffer(privkey),
        boost::asio::ssl::context::pem);
    context.use_certificate_chain(
        boost::asio::buffer(cert_chain));
}

void
start_server()
{
    using namespace boost;
    using namespace trezord;

    using server_type = api::connection_handler::server;

    trezord::core::kernel kernel;

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

    // thread pool
    auto thread_pool = make_shared<network::utils::thread_pool>(2);

    // https
    auto context = make_shared<boost::asio::ssl::context>(
        boost::asio::ssl::context::sslv23);
    configure_https(*context);

    // server
    server_type::options options{connection_handler};
    server_type server{
        options
            .reuse_address(true)
            .io_service(io_service)
            .thread_pool(thread_pool)
            .address(server_address)
            .port(server_port)
            .context(context)};

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

int
main(int argc, char *argv[])
{
    namespace po = boost::program_options;
    po::options_description desc("Options");
    desc.add_options()
       ("foreground,f", "run in foreground, don't fork into background")
       ("help,h", "produce help message")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    _START_EASYLOGGINGPP(argc, argv);
    configure_logging();

#ifdef __linux__
    if (!vm.count("foreground")) {
        if (daemon(0, 0) < 0) {
            LOG(ERROR) << "could not daemonize";
            exit(1);
        }
    }
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
