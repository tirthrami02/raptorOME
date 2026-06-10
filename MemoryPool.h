#pragma once

#include <iostream>
#include <cstdint>

class MemoryPool
{
private:
    size_t capacity;

    // Actual contiguous block of raw memory for our Orders
    Order *pool;
    // A simple, fast stack holding pointers to the free Orders
    Order **freeStack;

    size_t top;

public:
    // this has at application startup, BEFORE trading begins
    MemoryPool(uint64_t size) : capacity(size), top(size) {
        pool = new Order[capacity];
        freeStack = new Order *[capacity];

        for (size_t i = 0; i < capacity;i++)
        {
            freeStack[i] = &pool[i];
        }
    }
    ~MemoryPool()
    {
        delete[] pool;
        delete[] freeStack;
    }

    // O(1) Allocation: Just pop a pointer off our internal stack
    Order* allocate()
    {
        if(top == 0)
        {
            // Happenes when you miscalculated daily volume.
            std::cerr << "FATAL ERROR : Memory Pool Exhausted !" << std::endl;
            return nullptr;
        }
        return freeStack[--top];
    }

    // O(1) Deallocation: Push the order back in stack
    void deallocate(Order* order)
    {
        if(top == capacity)
        {
            std::cerr << "FATAL ERROR : Memory Pool Overflow !" << std::endl;
        }
        freeStack[top++] = order;
    }
};