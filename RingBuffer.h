#pragma once
#include <atomic>
#include <cstddef>
#include <chrono>
#include "OrderBook.h"

// Building SPSC : Single Producer - Single Consumer ring buffer

struct OrderRequest
{
    uint64_t id;
    uint64_t price;
    uint64_t quantity;
    Side side;
    std::chrono::time_point<std::chrono::high_resolution_clock> creationTime;
};

// Using template so that compiler hardcodes the size as in constexpr
template <size_t Capacity>
class RingBuffer
{
private:
    OrderRequest buffer[Capacity];

    // alignas prevents False Sharing
    // Forces CPU to put head and tail in different Level 1 caches
    alignas(64) std::atomic<size_t> head{0}; // consumers
    alignas(64) std::atomic<size_t> tail{0}; // producers
    // head and tail will have zero hardware collision.

public:
    /*
        memory_order* codes are the fastest way to share data.
        It means Do not reorder my assembly instrcution but don't
        bother to syncing with the entire system.
    */

    // Called only by Network threads
    bool push(OrderRequest request)
    {
        // Relaxed load because only producers modifies the tail
        size_t currentTail = tail.load(std::memory_order_relaxed);
        size_t nextTail = (currentTail + 1) % Capacity;

        // Acquire load to see most recent head updated by the consumer
        if (nextTail == head.load(std::memory_order_acquire))
        {
            // Buffer is full, the engine is falling behind.
            return false;
        }

        buffer[currentTail] = request;

        // Release the store to ensure that order data is visible after tail is updated
        tail.store(nextTail, std::memory_order_release);

        return true;
    }

    // Called only by Matching threads
    bool pop(OrderRequest &request)
    {
        // Relaxed the load becuse only producers modifies the head
        size_t currentHead = head.load(std::memory_order_relaxed);
        size_t nextHead = (currentHead + 1) % Capacity;

        // Acquire load to see most recent tail updated by the producers
        if (currentHead == tail.load(std::memory_order_acquire))
        {
            // Buffer is empty, Nothing to do.
            return false;
        }

        request = buffer[currentHead];

        // Release the store to ensure we finished before the head updates
        head.store(nextHead, std::memory_order_release);
        return true;
    }
};