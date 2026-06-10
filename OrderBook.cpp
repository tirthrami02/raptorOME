#include "OrderBook.h"
#include <algorithm>

void OrderBook::addOrder(uint64_t id, uint64_t price, uint64_t quantity, Side side)
{
    // O(1) Allocation : Grabbing free struct from our custom memory pool
    Order *newOrder = memoryPool.allocate();
    if(!newOrder)
    {
        std::cerr << "FATAL ERROR : Pool is Exhausted." << std::endl;
        return;
    }

    // Populate the order data
    newOrder->price = price;
    newOrder->orderId = id;
    newOrder->quantity = quantity;
    newOrder->side = side;

    // O(1) Level Lookup: Direct array indexing
    // We get a reference to the exact Level queue for this price
    Level &level = (side == Side::BUY) ? bids[price] : asks[price];

    // O(1) : Classic linked list operations
    if(level.tail == nullptr)
    {
        // This is the first order at this point
        newOrder->prev = nullptr;
        newOrder->next = nullptr;
        level.head = newOrder;
        level.tail = newOrder;
    }
    else
    {
        // There are already orders here. Put this one at the back of the line.
        newOrder->prev = level.tail;
        newOrder->next = nullptr;
        level.tail->next = newOrder;
        level.tail = newOrder;
    }

    // Updating total quantities
    level.totalQuantities += quantity;

    // Tracking maximum Bid Price and minimum Asks Price
    if(side == Side::BUY)
    {
        if(price > bestBidPrice)
            bestBidPrice = price;
    }
    else{
        if(price < bestAskPrice)
            bestAskPrice = price;
    }
}

void OrderBook::matchOrder()
{
    // Keep matching as long as buyers are willing to pay what sellers are asking
    while(true)
    {
        // Check if book is crosses
        if(bestBidPrice == 0 || bestAskPrice == UINT64_MAX || bestBidPrice < bestAskPrice)
        {
            break;
        }

        Level &bidLevel = bids[bestBidPrice];
        Level &askLevel = asks[bestAskPrice];

        Order *bidOrder = bidLevel.head;
        Order *askOrder = askLevel.head;

        // FailSafe : If a level is marked as best but has no orders, update trackers
        if(!bidOrder || !askOrder)
        {
            // In a production system, we'd scan down to find the real best price here.
            break;
        }

        // Finding minimum shares can be traded.
        uint64_t matchQuantity = std::min(bidOrder->quantity, askOrder->quantity);

        // Subtracting the traded quanitites from orders and levels
        bidOrder->quantity -= matchQuantity;
        askOrder->quantity -= matchQuantity;
        bidLevel.totalQuantities -= matchQuantity;
        askLevel.totalQuantities -= matchQuantity;
        
        // Cleanup : If a bidOrder is fully filled then remove it and deallocate.
        if(bidOrder->quantity == 0)
        {
            bidLevel.head = bidOrder->next;
            if(bidLevel.head)
            {
                bidLevel.head->prev = nullptr;
            }
            else
            {
                bidLevel.tail = nullptr;    // level is empty
                // find next active bid price
                while(bestBidPrice > 0 && bids[bestBidPrice].head == nullptr){
                    bestBidPrice--;
                }
            }
            memoryPool.deallocate(bidOrder);
        }

        // Cleanup : If a askOrder is fully filled then remove it and deallocate.
        if(askOrder->quantity == 0)
        {
            askLevel.head = askOrder->next;
            if(askLevel.head)
            {
                askLevel.head->prev = nullptr;
            }
            else
            {
                askLevel.tail = nullptr;    // level is empty
                // find next active best ask price
                while(bestAskPrice < MAX_PRICE_LEVELS - 1 && asks[bestAskPrice].head == nullptr)
                {
                    bestAskPrice++;
                }
            }
            memoryPool.deallocate(askOrder);
        }
    }
}