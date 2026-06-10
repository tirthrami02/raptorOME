#pragma once

#include <cstdint>
#include <iostream>
enum class Side
{
    BUY,
    SELL
};

struct Order
{
    uint64_t orderId; // fixed width integers
    uint64_t price;
    uint64_t quantity;
    Side side;

    // pointers for our custom linked list later ( to avoid std::list overhead, list scatters the memory )
    Order *next;
    Order *prev;
};

struct Level
{
    uint64_t price;
    uint64_t totalQuantities;

    Order *head;
    Order *tail;
};

// Just compiling OrerBook.cpp and main.cpp which is fo test, not compiling every files here,
// their #include code will pull the code from header, here is classic C++ cycling happening.
#include "MemoryPool.h"

class OrderBook
{
private:
    static constexpr size_t MAX_PRICE_LEVELS = 1000000;
    uint64_t bestBidPrice = 0;
    uint64_t bestAskPrice = UINT64_MAX;

    MemoryPool memoryPool{1000000};
    Level bids[MAX_PRICE_LEVELS];
    Level asks[MAX_PRICE_LEVELS];

public:

    OrderBook()
    {
        for (size_t i = 0; i < MAX_PRICE_LEVELS;i++)
        {
            bids[i] = {i, 0, nullptr, nullptr};
            asks[i] = {i, 0, nullptr, nullptr};
        }
    }

    void addOrder(uint64_t id, uint64_t price, uint64_t quantity, Side side);
    void matchOrder();
};