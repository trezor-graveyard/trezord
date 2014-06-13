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
            action action_ = dispatch(request);

            if (action_) {
                action_(this, request, response);
            } else {
                response = server::response::stock_reply(server::response::not_found);
            }
        }

        void
        log(...)
        {
            // do nothing for now
        }

    private:

        typedef std::function<void (handler*, server::request&, server::response&)> action;
        typedef std::string req_method;
        typedef std::string req_destination;
        struct action_route
        {
            req_method method;
            req_destination destination;
            action action;
        };

        const action_route action_routes[6] = {
            { "GET",  "/",          &handler::handle_index },
            { "GET",  "/devices",   &handler::handle_devices },
            { "POST", "/configure", &handler::handle_configure },
            { "POST", "/open",      &handler::handle_open },
            { "POST", "/call",      &handler::handle_call },
            { "POST", "/close",     &handler::handle_close }
        };

        action
        dispatch(server::request &request)
        {
            for (auto route: action_routes) {
                if (request.method == route.method &&
                    request.destination == route.destination) {
                    return route.action;
                }
            }
            return 0;
        }

        void
        handle_index(server::request &request,
                     server::response &response)
        {}

        void
        handle_devices(server::request &request,
                       server::response &response)
        {}

        void
        handle_configure(server::request &request,
                        server::response &response)
        {}

        void
        handle_open(server::request &request,
                    server::response &response)
        {}

        void
        handle_close(server::request &request,
                     server::response &response)
        {}

        void
        handle_call(server::request &request,
                    server::response &response)
        {}
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
