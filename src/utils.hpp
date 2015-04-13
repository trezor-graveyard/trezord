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

#include <sstream>
#include <queue>

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/future.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/hex.hpp>

namespace trezord
{
namespace utils
{

template <typename T>
struct blocking_queue
{
    void
    put(T item)
    {
        boost::unique_lock<boost::mutex> lock{mutex};
        queue.push(std::move(item));
        cond_var.notify_one();
    }

    T
    take()
    {
        boost::unique_lock<boost::mutex> lock{mutex};
        while (queue.empty()) {
            cond_var.wait(lock);
        }
        T item = std::move(queue.front());
        queue.pop();
        return std::move(item);
    }

private:

    std::queue<T> queue;
    boost::mutex mutex;
    boost::condition_variable cond_var;
};

struct async_executor
{
    async_executor()
        : thread{boost::bind(&async_executor::run, this)}
    { }

    ~async_executor()
    {
        thread.interrupt();
        thread.join();
    }

    template<typename Callable>
    boost::unique_future<typename std::result_of<Callable()>::type>
    add(Callable callable)
    {
        using result_type = typename std::result_of<Callable()>::type;
        using task_type = boost::packaged_task<result_type>;

        auto task = std::make_shared<task_type>(callable);
        auto future = task->get_future();

        queue.put(std::bind(&task_type::operator(), task));
        return std::move(future);
    }

    template<typename Callable>
    typename std::result_of<Callable()>::type
    await(Callable callable)
    {
        return add(callable).get();
    }

private:

    void
    run()
    {
        while (!boost::this_thread::interruption_requested()) {
            queue.take()();
        }
    }

    blocking_queue<
        std::function<void()> > queue;
    boost::thread thread;
};

std::string
hex_encode(std::string const &str)
{
    try {
        std::ostringstream stream;
        std::ostream_iterator<char> iterator{stream};
        boost::algorithm::hex(str, iterator);
        std::string hex{stream.str()};
        boost::algorithm::to_lower(hex);
        return hex;
    }
    catch (std::exception const &e) {
        throw std::invalid_argument{"cannot encode value to hex"};
    }
}

std::string
hex_decode(std::string const &hex)
{
    try {
        std::ostringstream stream;
        std::ostream_iterator<char> iterator{stream};
        boost::algorithm::unhex(hex, iterator);
        return stream.str();
    }
    catch (std::exception const &e) {
        throw std::invalid_argument{"cannot decode value from hex"};
    }
}

}
}
