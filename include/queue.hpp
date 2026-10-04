#pragma once
#include <vector>
#include <atomic>

template<typename T>

class SPSCQueue {
private:
    size_t capacity; // always a power of two: push/pop mask with (capacity - 1)
    std::vector<T> buffer;

    alignas(64)std::atomic<size_t> writeIndex{0};
    alignas(64)std::atomic<size_t> readIndex{0};
    alignas(64)std::atomic<bool> isReading{false};
public:
    SPSCQueue(size_t size) : capacity(size + 1), buffer(size + 1) {}

    // Re-sizes the ring. NOT thread-safe: only valid before the producer and
    // consumer threads start. `size` must be (power of two - 1), as in the ctor.
    void reset(size_t size) {
        capacity = size + 1;
        buffer.assign(capacity, T{});
        writeIndex.store(0, std::memory_order_relaxed);
        readIndex.store(0, std::memory_order_relaxed);
    }
    
    bool push(T& order) {
        size_t currentTail = writeIndex.load(std::memory_order_relaxed);
        size_t nextTail = (currentTail + 1) & (capacity - 1);

        if (nextTail == readIndex.load(std::memory_order_acquire)) {
            return false;
        }

        buffer[currentTail] = order;
        
        writeIndex.store(nextTail, std::memory_order_release);
        return true;
    }
    
    bool pop(T &order) {
        size_t currentHead = readIndex.load(std::memory_order_relaxed);
        size_t nextHead = (currentHead + 1) & (capacity - 1);

        if (currentHead == writeIndex.load(std::memory_order_acquire)) {
            return false;
        }

        order = buffer[currentHead];
        readIndex.store(nextHead, std::memory_order_release);
        return true;
    }
};