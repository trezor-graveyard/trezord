#define _ELPP_DEFAULT_LOG_FILE "logs/trezord.log"

#include <easylogging++.h>

#include "utils.hpp"
#include "wire.hpp"
#include "core.hpp"
#include "api.hpp"

_INITIALIZE_EASYLOGGINGPP

int
main(int argc, char *argv[])
{
    using namespace trezord;

    try {
        core::kernel kernel;

        api::request_handler<
            api::connection_handler::server
            > request_handler(kernel);

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
    }
    catch (std::exception const &e) {
        LOG(FATAL) << e.what();
        return 1;
    }
    return 0;
}
