#include "wire.hpp"
#include "core.hpp"
#include "api.hpp"

int
main(int argc, char *argv[])
{
    using namespace trezord;

    try {
        auto thread_pool = boost::make_shared<
            boost::network::utils::thread_pool>(2);

        core::kernel kernel;

        api::request_handler<
            api::connection_handler::server
            > request_handler(kernel);

        api::connection_handler connection_handler(request_handler);
        api::connection_handler::server::options options(connection_handler);
        api::connection_handler::server server(
            options
            .thread_pool(thread_pool)
            .address("127.0.0.1")
            .port("8000"));

        server.run();
    }
    catch (std::exception const &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
