#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

namespace trezord
{
namespace protobuf
{

template <typename I>
struct blocking_queue
{
    typedef I item_type;

    void
    put(const item_type &item)
    {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(item);
        cond_var.notify_one();
    }

    void
    take(item_type &item)
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
    std::queue<item_type> queue;
};

}
}
