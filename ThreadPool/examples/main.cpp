#include "thread_pool.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>
#include <thread>
#include <cmath>
#include <atomic>

int add(int a, int b)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return a + b;
}

double complex_calculation(int n)
{
    double result = 0.0;
    for (int i = 0; i < n; ++i) {
        result += std::sin(i * 0.1) * std::cos(i * 0.2);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return result;
}

void risky_task(int id)
{
    if (id % 5 == 0) {
        throw std::runtime_error("Task " + std::to_string(id) + " failed on purpose");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    std::cout << "Task " << id << " completed successfully\n";
}

void example_with_future()
{
    std::cout << "\n=== Example: Using Futures to Collect Results ===\n";

    ThreadPool pool(4);

    std::vector<std::future<int>> futures;
    for (int i = 0; i < 10; ++i) {
        auto future =  pool.enqueue([i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(i * 10));
            return i * i;
        });
        futures.push_back(std::move(future));
    }

    pool.waitAll();

    std::vector<int> results;
    for (auto& future : futures) {
        results.push_back(future.get());
    }

    std::cout << "Results: ";
    for (auto& r : results) {
        std::cout << r << " ";
    }
    std::cout << std::endl;
}

void example_with_exceptions()
{
    std::cout << "\n=== Example: Exception Handling ===\n";
    ThreadPool pool(2);
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 10; ++i) {
        try {
            auto future = pool.enqueue(risky_task, i);
            futures.push_back(std::move(future));
        } catch (const std::exception& e) {
            std::cerr << "Failed to enqueue task " << i << ": " << e.what() << std::endl;
        }
    }

    for (size_t i = 0; i < futures.size(); ++i) {
        try {
            futures[i].get();
        } catch (const std::exception& e) {
            std::cerr << "Task " << i << " threw exception: " << e.what() << std::endl;
        }
    }
}

void example_parallel_algorithm()
{
    std::cout << "\n=== Example: Parallel Algorithm ===\n";

    const int num_elements = 1000;
    ThreadPool pool(std::thread::hardware_concurrency());
    std::atomic<int> total_sum {0};
    
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i <= num_elements; ++i) {
        pool.enqueue([i, &total_sum]() {
            int square = i * i;
            total_sum += square;
        });
    }

    pool.waitAll();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "并行计算 " << num_elements << " 个数的平方和\n";
    std::cout << "结果: " << total_sum.load() << std::endl;
    std::cout << "耗时: " << duration.count() << " 微秒\n";
    
    int expected = num_elements * (num_elements + 1) * (2 * num_elements + 1) / 6;
    std::cout << "期望值: " << expected << std::endl;
    std::cout << "验证: " << (total_sum.load() == expected ? " 正确" : " 错误") << std::endl;
}

void example_performance_comparison()
{
    std::cout << "\n=== 性能对比 ===\n";
    const int num_tasks = 100;
    std::cout << "开始顺序执行...\n";

    auto seq_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_tasks; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    auto seq_end = std::chrono::high_resolution_clock::now();
    auto seq_duration = std::chrono::duration_cast<std::chrono::milliseconds>(seq_end - seq_start);
    std::cout << "顺序执行耗时: " << seq_duration.count() << " 毫秒\n";

    std::cout << "开始并行执行...\n";
    ThreadPool pool(4);
    auto parallel_start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_tasks; ++i) {
        pool.enqueue([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });
    }
    pool.waitAll();
    auto parallel_end = std::chrono::high_resolution_clock::now();
    auto parallel_duration = std::chrono::duration_cast<std::chrono::milliseconds>(parallel_end - parallel_start);
    std::cout << "并行执行耗时: " << parallel_duration.count() << " 毫秒\n";
    std::cout << "加速比: " << (static_cast<double>(seq_duration.count()) / parallel_duration.count()) << "x\n";
}

int main()
{
    std::cout << "ThreadPool Example Program\n";
    std::cout << "Hardware concurrency: " << std::thread::hardware_concurrency() << std::endl;

    {
        std::cout << "\n=== Example 1: Basic Usage ===\n";

        ThreadPool pool(std::thread::hardware_concurrency());
        std::cout << "Thread pool created with " << pool.threadCount() << " threads\n";

        auto future1 = pool.enqueue(add, 10, 20);
        auto future2 = pool.enqueue(complex_calculation, 1000);
        auto future3 = pool.enqueue([]() {
            return std::string("Hello from thread pool lambda!");
        });

        std::cout << "10 + 20 = " << future1.get() << std::endl;
        std::cout << "Complex calculation result: " << future2.get() << std::endl;
        std::cout << future3.get() << std::endl;

        pool.waitAll();
        std::cout << "All tasks completed\n";
    }

    example_with_future();
    example_with_exceptions();
    example_parallel_algorithm();
    example_performance_comparison();

    {
        std::cout << "\n=== 线程池状态管理 ===\n";
        ThreadPool pool(2);

        std::cout << "当前队列任务: " << pool.taskCount() << std::endl;
        std::cout << "线程池是否运行: " << (pool.isRunning() ? "是" : "否") << std::endl;

        for (int i = 0; i < 5; ++i) {
            pool.enqueue([i]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                std::cout << "任务 " << i << " 完成\n";
            });
        }

        std::cout << "提交后队列任务数: " << pool.taskCount() << std::endl;

        pool.waitAll();
        std::cout << "等待后队列任务数: " << pool.taskCount() << std::endl;

        pool.stop();
        std::cout << "停止后线程池是否运行: " << (pool.isRunning() ? "是" : "否") << std::endl;

        try {
            pool.enqueue([]() {
                std::cout << "这个任务不应该执行";
            });
        } catch (const std::runtime_error& e) {
            std::cout << "捕获到预期异常: " << e.what() << std::endl;
        }
    }
    
    return 0;
}