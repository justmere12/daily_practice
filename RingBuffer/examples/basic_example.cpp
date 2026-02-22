#include "ring_buffer.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <cassert>

void test_basic_operations()
{
    std::cout << "=== 测试基础操作 ===" << std::endl;

    RingBuffer<int, 8> ring;
    std::cout << "初始状态: empty=" << ring.empty()
              << ", full=" << ring.full()
              << ", size=" << ring.size()
              << ", available=" << ring.available() << std::endl;
    
    std::cout << "\n测试push操作: " << std::endl;
    for (int i = 0; i < 5; ++i) {
        bool success = ring.push(i);
        std::cout << "  push(" << i << "): " << (success ? "成功" : "失败")
                  << ", size=" << ring.size()
                  << ", available=" << ring.available() << std::endl;
    }

    std::cout << "\n测试peek操作: " << std::endl;
    int peek_value;
    if (ring.peek(peek_value)) {
        std::cout << "  peek: " << peek_value << std::endl;
    }

    std::cout << "\n测试pop操作: " << std::endl;
    int pop_value;
    while (ring.pop(pop_value)) {
        std::cout << "  pop: " << pop_value << ", size=" << ring.size() << std::endl;
    }

    std::cout << "\n测试边界条件: " << std::endl;
    RingBuffer<int, 4> small_ring;

    for (int i = 0; i < 4; ++i) {
        small_ring.push(i);
    }
    std::cout << "  填满后: full=" << small_ring.full() << ", empty=" << small_ring.empty() << std::endl;

    bool result = small_ring.push(99);
    std::cout << "  满时push: " << (result ? "成功" : "失败") << std::endl;

    while (small_ring.pop(pop_value)) {}
    std::cout << "  清空后: full=" << small_ring.full() << ", empty=" << small_ring.empty() << std::endl;

    result = small_ring.pop(pop_value);
    std::cout << "  空时pop: " << (result ? "成功" : "失败") << std::endl;

    std::cout << std::endl;
}

void test_thread_safety()
{
    std::cout << "=== 测试线程安全性 ===" << std::endl;

    constexpr size_t CAPACITY = 1024;
    constexpr int TOTAL_ITEMS = 10000;

    RingBuffer<int, CAPACITY> ring;
    std::atomic<int> produced {0};
    std::atomic<int> consumed {0};

    auto producer = [&]() {
        for (int i = 0; i < TOTAL_ITEMS; ++i) {
            while (!ring.push(i)) {
                std::this_thread::yield();
            }
            produced.fetch_add(1, std::memory_order_relaxed);
        }
    };

    auto consumer = [&]() {
        int value;
        int local_count = 0;
        while (local_count < TOTAL_ITEMS) {
            if (ring.pop(value)) {
                ++local_count;
                consumed.fetch_add(1, std::memory_order_relaxed);
            } else {
                std::this_thread::yield();
            }
        }
    };

    auto start = std::chrono::high_resolution_clock::now();

    std::thread t1(producer);
    std::thread t2(consumer);

    t1.join();
    t2.join();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "生产数量: " << produced.load() << std::endl;
    std::cout << "消费数量: " << consumed.load() << std::endl;
    std::cout << "最终队列大小: " << ring.size() << std::endl;
    std::cout << "耗时: " << duration.count() << std::endl;
    std::cout << "吞吐量: " << (TOTAL_ITEMS * 1000.0 / duration.count()) << " 操作/秒" << std::endl;

    if (produced == TOTAL_ITEMS && consumed == TOTAL_ITEMS && ring.empty()) {
        std::cout << "线程安全测试通过" << std::endl;
    } else {
        std::cout << "线程安全测试失败" << std::endl;
    }
    std::cout << std::endl;
}

void test_batch_operations()
{
    std::cout << "=== 测试批量操作 ===" << std::endl;

    RingBuffer<int, 16> ring;

    std::vector<int> push_data = {1, 2, 3, 4, 5, 6, 7, 8};
    std::vector<int> pop_data(push_data.size());

    size_t pushed = ring.push_batch(push_data.data(), push_data.size());
    std::cout << "批量push: 尝试" << push_data.size() << " 个，成功" << pushed << " 个" << std::endl;

    size_t popped = ring.pop_batch(pop_data.data(), pop_data.size());
    std::cout << "批量pop: 尝试" << pop_data.size() << " 个，成功" << popped << "个" << std::endl;

    bool success = true;
    for (size_t i = 0; i < popped; ++i) {
        if (push_data[i] != pop_data[i]) {
            success = false;
            std::cout << "  数据不一致: " << push_data[i] << " != " << pop_data[i] << std::endl;
        }
    }

    if (success && pushed == popped) {
        std::cout << "批量操作测试通过" << std::endl;
    } else {
        std::cout << "批量操作测试失败" << std::endl;
    }

    std::cout << std::endl;
}

int main()
{
    std::cout << "RingBuffer 基本示例\n" << std::endl;

    test_basic_operations();
    test_thread_safety();
    test_batch_operations();

    std::cout << "所有测试完成" << std::endl;
    return 0;
}