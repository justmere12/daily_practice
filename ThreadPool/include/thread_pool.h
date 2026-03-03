#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <atomic>

class ThreadPool {
public:
    ThreadPool() = delete;
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency());

    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&& other) noexcept;
    ThreadPool& operator=(ThreadPool&& other) noexcept;

    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;

    size_t taskCount() const;

    size_t threadCount() const;

    void waitAll();

    void stop();

    bool isRunning() const { return running_;}

private:
    void workerThread();
    void cleanup();
    void transferFrom(ThreadPool&& other) noexcept;

    std::queue<std::function<void()>> tasks_;
    std::vector<std::thread> workers_;
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::condition_variable condition_empty_;
    std::atomic<bool> running_ {false};
    std::atomic<size_t> active_tasks_ {0};
};

inline ThreadPool::ThreadPool(ThreadPool&& other) noexcept : running_(false)
{
    transferFrom(std::move(other));
}

inline ThreadPool& ThreadPool::operator=(ThreadPool&& other) noexcept
{
    if (this != &other) {
        cleanup();
        transferFrom(std::move(other));
    }
    return *this;
}

template<typename F, typename... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> res = task->get_future();

    {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        if (!running_) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        tasks_.emplace([task]() { (*task)(); });
    }
    condition_.notify_one();

    return res;
}

#endif