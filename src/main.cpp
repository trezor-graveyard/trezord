#include "wire.hpp"
#include "core.hpp"
#include "api.hpp"

int
main(int argc, char *argv[])
{
    using namespace trezord;
    using namespace boost;

    try {
        auto io_service = make_shared<asio::io_service>();
        auto io_work = make_shared<asio::io_service::work>(ref(*io_service));

        auto thread_group = make_shared<boost::thread_group>();
        thread_group->create_thread(bind(&asio::io_service::run, io_service));

        auto thread_pool = make_shared<
            network::utils::thread_pool>(2, io_service, thread_group);

        api::handler handler;
        api::handler::server::options options(handler);
        api::handler::server server(options
                                    .io_service(io_service)
                                    .address("127.0.0.1")
                                    .port("8000"));

        server.run();
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
