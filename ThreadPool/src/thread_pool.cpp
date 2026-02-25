#include "thread_pool.h"
#include <iostream>

ThreadPool::ThreadPool(size_t threads)
{
    if (threads == 0) {
        threads = std::thread::hardware_concurrency();
        if (threads == 0) threads = 4;
    }
    
    workers_.reserve(threads);
    for (size_t i = 0; i < threads; ++i) {
        workers_.emplace_back([this] { workerThread(); });
    }
}

ThreadPool::~ThreadPool()
{
    cleanup();
}

void ThreadPool::cleanup()
{
    if (!running_) return;
    
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        running_ = false;
    }

    condition_.notify_all();

    for (std::thread &worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    workers_.clear();
    while (!tasks_.empty()) {
        tasks_.pop();
    }
}

void ThreadPool::transferFrom(ThreadPool&& other) noexcept
{
    try {
        std::unique_lock<std::mutex> lock1(queue_mutex_, std::defer_lock);
        std::unique_lock<std::mutex> lock2(other.queue_mutex_, std::defer_lock);
        std::lock(lock1, lock2);

        workers_ = std::move(other.workers_);
        tasks_ = std::move(other.tasks_);
        running_ = other.running_.load();
        active_tasks_ = other.active_tasks_.load();

        other.running_ = false;
        other.active_tasks_ = 0;
    } catch (...) {
        running_ = false;
        workers_.clear();
        while (!tasks_.empty()) tasks_.pop();
    }
}

void ThreadPool::workerThread()
{
    while (running_) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            condition_.wait(lock, [this] {
                return !running_ || !tasks_.empty();
            });

            if (!running_ && tasks_.empty()) {
                return;
            }
            task = std::move(tasks_.front());
            tasks_.pop();

            ++active_tasks_;
        }
        try {
            task();
        } catch (...) {
            std::cerr << "Exception in thread pool task" << std::endl;
        }

        --active_tasks_;
        condition_empty_.notify_one();
    }
}

size_t ThreadPool::taskCount() const
{
    std::unique_lock<std::mutex> lock(queue_mutex_);
    return tasks_.size();
}

size_t ThreadPool::threadCount() const
{
    std::unique_lock<std::mutex> lock(queue_mutex_);
    return workers_.size();
}

void ThreadPool::waitAll()
{
    std::unique_lock<std::mutex> lock(queue_mutex_);
    condition_empty_.wait(lock, [this] {
        return tasks_.empty() && active_tasks_ == 0;
    });
}

void ThreadPool::stop()
{
    cleanup();
}