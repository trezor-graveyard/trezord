#include <hidapi.h>

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

#include <boost/network/protocol/http/server.hpp>
#include <iostream>
#include <string>
#include <regex>

namespace hid
{
    struct trezor_device
    {
        typedef unsigned char char_type;
        typedef std::size_t size_type;

        typedef std::runtime_error open_error;
        typedef std::runtime_error read_error;
        typedef std::runtime_error write_error;

        trezor_device(const trezor_device&) = delete;
        trezor_device &operator=(const trezor_device&) = delete;

        trezor_device(const char *path)
        {
            hid = hid_open_path(path);
            if (!hid) {
                throw new open_error("HID device open failed");
            }

            hid_set_nonblocking(hid, 0); // always block on read

            unsigned char uart[] = {0x41, 0x01}; // enable UART
            unsigned char txrx[] = {0x43, 0x03}; // purge TX/RX FIFOs
            hid_send_feature_report(hid, uart, 2);
            hid_send_feature_report(hid, txrx, 2);
        }

        ~trezor_device()
        {
            hid_close(hid);
        }

        void
        read_buffered(char_type *data, size_type len)
        {
            if (read_buffer.empty()) {
                buffer_report();
            }
            size_type n = read_report_from_buffer(data, len);
            if (n < len) {
                read_buffered(data + n, len - n);
            }
        }

        void
        write(const char_type *data, size_type len)
        {
            size_type n = write_report(data, len);
            if (n < len) {
                write(data + n, len - n);
            }
        }

    private:

        void
        buffer_report()
        {
            report_type report;
            int r = hid_read(hid,
                             report.data(),
                             report.size());

            if (r < 0) {
                throw new read_error("HID device read failed");
            }
            if (r > 0) {
                // copy to the buffer, skip the report number
                char_type rn = report[0];
                size_type n = std::min(static_cast<size_type>(rn),
                                       static_cast<size_type>(r - 1));
                std::copy(report.begin() + 1,
                          report.begin() + n,
                          std::back_inserter(read_buffer));
            }
        }

        size_type
        read_report_from_buffer(char_type *data, size_type len)
        {
            size_type n = std::min(read_buffer.size(), len);
            auto r1 = read_buffer.begin();
            auto r2 = read_buffer.begin() + n;

            std::copy(r1, r2, data); // copy to data
            read_buffer.erase(r1, r2); // shift from buffer

            return n;
        }

        size_type
        write_report(const char_type *data, size_type len)
        {
            report_type report;
            report[0] = report.size() - 1; // rest is 0x00

            size_type n = std::min(report.size() - 1, len);
            std::copy(data,
                      data + n,
                      report.begin() + 1); // copy behind report number

            int r = hid_write(hid,
                              report.data(),
                              report.size());
            if (r < 0) {
                throw new write_error("HID device write failed");
            }
            if (r < report.size()) {
                throw new write_error("HID device write was insufficient");
            }

            return n;
        }

        typedef std::vector<char_type> buffer_type;
        typedef std::array<char_type, 64> report_type;

        hid_device *hid;
        buffer_type read_buffer;
    };
}

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
            { "GET",  "/enumerate",  &handler::handle_enumerate },
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
                bool m = std::regex_match(request.method, route.method);
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
        handle_enumerate(action_params &params,
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
