#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <atomic>
#include <cstddef>
#include <type_traits>

template<typename T, size_t Capacity>
class RingBuffer {
private:
    alignas(64) std::atomic<size_t> head_;
    alignas(64) std::atomic<size_t> tail_;
    alignas(64) T buffer_[Capacity];

    static constexpr size_t mask_ = Capacity - 1;
public:
    RingBuffer() noexcept : head_(0), tail_(0) {}

    ~RingBuffer() {
        clear();
    }

    RingBuffer(const RingBuffer&&) = delete;
    RingBuffer& operator=(const RingBuffer&&) = delete;
    RingBuffer(RingBuffer&&) = delete;
    RingBuffer& operator=(RingBuffer&&) = delete;

    bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }

    bool full() const noexcept {
        const size_t head = head_.load(std::memory_order_acquire);
        const size_t tail = tail_.load(std::memory_order_acquire);
        return (head - tail) >= Capacity;
    }

    template<typename U>
    bool push(U&& value) noexcept {
        static_assert(std::is_nothrow_constructible<T, U&&>::value, "T must be nothrow constructible from U&&");

        size_t head = head_.load(std::memory_order_relaxed);
        size_t tail = tail_.load(std::memory_order_acquire);

        if ((head - tail) >= Capacity) {
            return false;
        }

        new (&buffer_[head & mask_]) T(std::forward<T>(value));

        head_.store(head + 1, std::memory_order_release);
        return true;
    }

    bool pop(T& value) noexcept {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t head = head_.load(std::memory_order_acquire);
        
        if (head == tail) {
            return false;
        }
    
        T* elem = &buffer_[tail & mask_];
        value = std::move(*elem);
        elem->~T();
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    size_t push_batch(const T* data, size_t count) noexcept {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t tail = tail_.load(std::memory_order_acquire);

        size_t available = Capacity - (head - tail);
        size_t to_push = (count < available) ? count : available;

        for (size_t i = 0; i < to_push; ++i) {
            new (&buffer_[(head + i) & mask_]) T(data[i]);
        }

        head_.store(head + to_push, std::memory_order_release);
        return to_push;
    }

    size_t pop_batch(T* data, size_t count) noexcept {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t head = head_.load(std::memory_order_acquire);

        size_t available = head - tail;
        size_t to_pop = (count < available) ? count : available;

        for (size_t i = 0; i < to_pop; ++i) {
            T* elem = &buffer_[(tail + i) & mask_];
            data[i] = std::move(*elem);
            elem->~T();
        }

        tail_.store(tail + to_pop, std::memory_order_release);
        return to_pop;
    }

    bool peek(T& value) const noexcept {
        size_t tail = tail_.load(std::memory_order_acquire);
        size_t head = head_.load(std::memory_order_acquire);

        if (head == tail) {
            return false;
        }

        value = buffer_[tail & mask_];
        return true;
    }

    size_t size() const noexcept {
        const size_t head = head_.load(std::memory_order_acquire);
        const size_t tail = tail_.load(std::memory_order_acquire);
        return head - tail;
    }

    size_t available() const noexcept {
        const size_t head = head_.load(std::memory_order_acquire);
        const size_t tail = tail_.load(std::memory_order_acquire);
        return Capacity - (head - tail);
    }
    
    constexpr size_t capacity() const noexcept {
        return Capacity;
    }

    void reset() noexcept {
        clear();
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }

private:
    void clear() noexcept {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t head = head_.load(std::memory_order_relaxed);

        while (tail != head) {
            buffer_[tail & mask_].~T();
            ++tail;
        }
    }
};

#endif