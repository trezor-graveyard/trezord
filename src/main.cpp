#include <boost/network/protocol/http/server.hpp>
#include <string>
#include <iostream>

namespace api
{
    struct handler
    {
        typedef boost::network::http::server<handler> server;

        void
        operator() (server::request &request,
                    server::response &response)
        {
            response = server::response::stock_reply(server::response::ok, "hello");
        }

        void
        log(...)
        {
            // do nothing for now
        }
    };
}

int
main(int argc, char *argv[])
{
    try {
        api::handler handler;
        api::handler::server::options options(handler);
        api::handler::server server(options
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
