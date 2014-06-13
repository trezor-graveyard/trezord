#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

#include <boost/network/protocol/http/server.hpp>
#include <iostream>
#include <string>
#include <regex>

namespace core
{
    template <typename Item>
    struct blocking_queue
    {
        void
        put(const Item &item)
        {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push(item);
            cond_var.notify_one();
        }

        void
        take(Item &item)
        {
            std::lock_guard<std::mutex> lock(mutex);
            while (queue.empty()) {
                cond_var.wait(lock);
            }
            item = queue.front();
            queue.pop();
        }

    private:

        std::mutex mutex;
        std::condition_variable cond_var;
        std::queue<Item> queue;
    };
}

namespace api
{
    struct handler
    {
        typedef boost::network::http::server<handler> server;

        void
        operator() (server::request &request,
                    server::response &response)
        {
            action_handler handler;
            action_params params;
            dispatch(request, handler, params);
            handler(this, params, request, response);
        }

        void
        log(...)
        {
            // do nothing for now
        }

    private:

        typedef std::smatch action_params;
        typedef std::function<void (handler*,
                                    action_params&,
                                    server::request&,
                                    server::response&)> action_handler;

        struct action_route
        {
            std::regex method;
            std::regex destination;
            action_handler handler;

            action_route(const std::string &m,
                         const std::string &d,
                         const action_handler &h)
                : method(m),
                  destination(d),
                  handler(h) {}
        };

        const action_route action_routes[7] = {
            { "GET",  "/",           &handler::handle_index },
            { "GET",  "/devices",    &handler::handle_devices },
            { "POST", "/configure",  &handler::handle_configure },
            { "POST", "/open/(.*)",  &handler::handle_open },
            { "POST", "/close/(.*)", &handler::handle_close },
            { "POST", "/call/(.*)",  &handler::handle_call },
            { ".*",   ".*",          &handler::handle_404 },
        };

        void
        dispatch(server::request &request,
                 action_handler &handler,
                 action_params &params)
        {
            for (auto route: action_routes) {
                bool m = std::regex_match(request.method,
                                          route.method);
                bool d = std::regex_match(request.destination, params,
                                          route.destination);
                if (m && d) {
                    handler = route.handler;
                    return;
                }
            }
            throw std::invalid_argument("Route not found");
        }

        void
        handle_index(action_params &params,
                     server::request &request,
                     server::response &response)
        {}

        void
        handle_devices(action_params &params,
                       server::request &request,
                       server::response &response)
        {}

        void
        handle_configure(action_params &params,
                         server::request &request,
                         server::response &response)
        {}

        void
        handle_open(action_params &params,
                    server::request &request,
                    server::response &response)
        {}

        void
        handle_close(action_params &params,
                     server::request &request,
                     server::response &response)
        {}

        void
        handle_call(action_params &params,
                    server::request &request,
                    server::response &response)
        {}

        void
        handle_404(action_params &params,
                   server::request &request,
                   server::response &response)
        {
            response = server::response::stock_reply(server::response::not_found);
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
