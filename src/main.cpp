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

#include <stdio.h>

#include <boost/chrono/chrono.hpp>
#include <boost/program_options.hpp>

#define _ELPP_THREAD_SAFE 1
#define _ELPP_FORCE_USE_STD_THREAD 1
#define _ELPP_NO_DEFAULT_LOG_FILE

#include <easylogging++.h>

#include "utils.hpp"
#include "hid.hpp"
#include "wire.hpp"
#include "core.hpp"
#include "http_client.hpp"
#include "http_server.hpp"
#include "http_api.hpp"

_INITIALIZE_EASYLOGGINGPP

static const auto server_port = 21324;
static const auto server_address = "127.0.0.1";

static const auto https_cert_uri = "https://mytrezor.s3.amazonaws.com/bridge/cert/server.crt";
static const auto https_privkey_uri = "https://mytrezor.s3.amazonaws.com/bridge/cert/server.key";

static const auto sleep_time = boost::chrono::seconds(10);

std::string
get_default_log_path()
{
#ifdef _WIN32
    if (auto app_data = std::getenv("APPDATA")) {
        return std::string{app_data} + "\\TREZOR Bridge\\trezord.log";
    }
    else {
        throw std::runtime_error{"environment variable APPDATA not found"};
    }
#elif __APPLE__
    if (auto home = std::getenv("HOME")) {
        return std::string{home} + "/Library/Logs/trezord.log";
    }
    else {
        throw std::runtime_error{"environment variable HOME not found"};
    }
#else
    return "/var/log/trezord.log";
#endif
}

void
configure_logging()
{
    const auto default_log_path = get_default_log_path();
    const auto default_format = "%datetime %level [%logger] [%thread] %msg";
    const auto max_log_file_size = "2097152"; // 2mb, log gets truncated after that

    el::Configurations cfg;
    cfg.setToDefault();
    cfg.setGlobally(el::ConfigurationType::MaxLogFileSize, max_log_file_size);
    cfg.setGlobally(el::ConfigurationType::Filename, default_log_path);
    cfg.setGlobally(el::ConfigurationType::Format, default_format);
    cfg.set(el::Level::Debug, el::ConfigurationType::Format, default_format);
    cfg.set(el::Level::Trace, el::ConfigurationType::Format, default_format);
    cfg.set(el::Level::Verbose, el::ConfigurationType::Format, default_format);

    // we need to explicitly construct all loggers used in the app
    // easylogging++ prints warnings if we dont
    el::Loggers::getLogger("http.client");
    el::Loggers::getLogger("http.server");
    el::Loggers::getLogger("core.device");
    el::Loggers::getLogger("core.config");
    el::Loggers::getLogger("core.kernel");
    el::Loggers::getLogger("wire.enumerate");

    // configure all created loggers
    el::Loggers::reconfigureAllLoggers(cfg);
}

void
start_server(std::string const &cert_uri,
             std::string const &privkey_uri,
             std::string const &address,
             unsigned int port)
{
    using namespace trezord;

    using std::bind;
    using std::placeholders::_1;
    using http_api::handler;

    auto cert = http_client::request_uri_to_string(cert_uri);
    auto privkey = http_client::request_uri_to_string(privkey_uri);

    http_api::handler api_handler{
        std::unique_ptr<core::kernel>{new core::kernel}};
    http_server::route_table api_routes = {
        {{"GET",  "/"},             bind(&handler::handle_index, &api_handler, _1) },
        {{"GET",  "/listen"},       bind(&handler::handle_listen, &api_handler, _1) },
        {{"GET",  "/enumerate"},    bind(&handler::handle_enumerate, &api_handler, _1) },
        {{"POST", "/listen"},       bind(&handler::handle_listen, &api_handler, _1) },
        {{"POST", "/configure"},    bind(&handler::handle_configure, &api_handler, _1) },
        {{"POST", "/acquire/([^/]+)"}, bind(&handler::handle_acquire, &api_handler, _1) },
        {{"POST", "/acquire/([^/]+)/([^/]+)"}, bind(&handler::handle_acquire, &api_handler, _1) },
        {{"POST", "/release/(.+)"}, bind(&handler::handle_release, &api_handler, _1) },
        {{"POST", "/call/(.+)"},    bind(&handler::handle_call, &api_handler, _1) },
        {{".*",   ".*"},            bind(&handler::handle_404, &api_handler, _1) }
    };
    http_server::server server{api_routes, [&] (char const *origin) {
            return api_handler.is_origin_allowed(origin);
        }};

    server.start(port, address.c_str(), privkey.c_str(), cert.c_str());
    for (;;) {
        boost::this_thread::sleep_for(sleep_time);
    }
    server.stop();
}

int
main(int argc, char *argv[])
{
    configure_logging();

    if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
        LOG(ERROR) << "could not init curl";
        return 1;
    }

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

#ifdef __linux__
    if (!vm.count("foreground")) {
        if (daemon(0, 0) < 0) {
            LOG(ERROR) << "could not daemonize";
            return 1;
        }
    }
#endif

start:
    try {
        start_server(https_cert_uri,
                     https_privkey_uri,
                     server_address,
                     server_port);
    }
    catch (std::exception const &e) {
        LOG(ERROR) << e.what();
        LOG(INFO) << "sleeping for " << sleep_time.count() << "s";
        boost::this_thread::sleep_for(sleep_time);
        goto start;
    }

    return 0;
}
