#include "OrderBook.h"
#include "RingBuffer.h"
#include <iostream>
#include <thread>
#include <chrono>

// Instantiate our lock-free queue (Size 1024)
// lock free bridge, so that every threads can access this.
RingBuffer<1024> orderQueue;
bool engineRunning = true; // Flag to shut down the engine

//
void networkThreadLoop()
{
    std::cout << "[NETWORK] Thread started on CPU core.\n";

    // Simulate incoming network traffic
    // Taking these as example as of now.
    // OrderRequest incomingOrders[] = {
    //     {101, 15000, 100, Side::SELL},
    //     {102, 15005, 200, Side::SELL},
    //     {201, 14995, 50, Side::BUY},
    //     // This massive BUY order will cross the spread and trigger trades!
    //     {202, 15010, 250, Side::BUY}};

    uint64_t prices[] = {15000, 15005, 14995, 15010};
    Side sides[] = {Side::SELL, Side::SELL, Side::BUY, Side::BUY};
    uint64_t quantities[] = {100, 200, 50, 250};

    // for (const auto &req : incomingOrders)
    for (int i = 0; i < 4; ++i)
    {
        OrderRequest req;
        req.id = 100 + i;
        req.price = prices[i];
        req.quantity = quantities[i];
        req.side = sides[i];

        // Stamp the exact time of creation
        req.creationTime = std::chrono::high_resolution_clock::now();

        // Spin-wait if the buffer is temporarily full
        while (!orderQueue.push(req))
        {
            // Performing Spin-Lock concept. It ensures zero microsecond latecny while space
            // is available.
            // Just continuing this buffer to ensure our transection is done
            // and then move forward.
            // avoiding closing the application of taking new process at this point
            // to main consistency within orders.
        }
        // std::cout << "[NETWORK] Pushed Order " << req.id << " to queue.\n";

        // Sleep for a millisecond just to simulate network delay between packets
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cout << "[NETWORK] All orders sent.\n";
}

void engineThreadLoop(OrderBook *engine)
{
    std::cout << "[ENGINE] Thread started and listening...\n";

    OrderRequest req;

    uint64_t totalLatency = 0;
    uint64_t processedCount = 0;

    while (engineRunning)
    {
        // Try to pop an order off the lock-free queue
        if (orderQueue.pop(req))
        {
            // We got an order! Add it to the OrderBook
            engine->addOrder(req.id, req.price, req.quantity, req.side);

            // Run the matcher to see if this new order triggered a trade
            engine->matchOrder();

            // Stamp the time after matching is complete
            auto processTime = std::chrono::high_resolution_clock::now();

            // Calculate nanoseconds
            auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(processTime - req.creationTime).count();

            std::cout << "[METRIC] Order " << req.id << " Processed. End-to-End Latency: "
                      << latency << " ns\n";

            totalLatency += latency;
            processedCount++;
        }
    }

    std::cout << "\n========================================\n";
    std::cout << "[BENCHMARK SUMMARY]\n";
    std::cout << "Total Orders Processed: " << processedCount << "\n";
    std::cout << "Total Time Taken: " << totalLatency << " ns\n";

    if (processedCount > 0)
    {
        std::cout << "Average Latency: " << (totalLatency / processedCount) << " ns\n";
    }
    std::cout << "========================================\n";

    std::cout << "[ENGINE] Shutting down.\n";
}

int main()
{
    std::cout << "--- Starting Multithreaded HFT Engine ---\n";

    // This avoids stack overflow - heap allocation
    OrderBook *engine = new OrderBook();

    // Starting Engine Thread
    std::thread engineThread(engineThreadLoop, engine);

    // Give the engine a millisecond to boot up and start listening
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Spawn the Network Thread
    std::thread networkThread(networkThreadLoop);

    // At this point : main, engine and network; all 3 timelines are running.
    // Wait for the Network Thread to finish sending all its packets
    networkThread.join();

    // Let the Engine process the final orders, then shut it down
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    engineRunning = false;
    engineThread.join();

    std::cout << "--- System Shutdown Successfully ---\n";

    delete engine;
    return 0;
}

// Old Code for test

// #include "OrderBook.h"
// #include "RingBuffer.h"
// #include <iostream>
// #include <thread>
// #include <chrono>

// // Instantiate our lock-free queue (Size 1024)
// RingBuffer<1024> orderQueue;
// bool engineRunning = true; // Flag to gracefully shut down the engine

// void networkThreadLoop() {
//     std::cout << "[NETWORK] Thread started on CPU core.\n";

//     // Simulate incoming network traffic
//     OrderRequest incomingOrders[] = {
//         {101, 15000, 100, Side::SELL},
//         {102, 15005, 200, Side::SELL},
//         {201, 14995, 50,  Side::BUY},
//         // This massive BUY order will cross the spread and trigger trades!
//         {202, 15010, 250, Side::BUY}
//     };

//     for (const auto& req : incomingOrders) {
//         // Spin-wait if the buffer is temporarily full
//         while (!orderQueue.push(req)) {
//             // In HFT, we just spin. We don't sleep.
//         }
//         std::cout << "[NETWORK] Pushed Order " << req.id << " to queue.\n";

//         // Sleep for a microsecond just to simulate network delay between packets
//         std::this_thread::sleep_for(std::chrono::microseconds(100));
//     }

//     std::cout << "[NETWORK] All orders sent.\n";
// }

// void engineThreadLoop(OrderBook* engine) {
//     std::cout << "[ENGINE] Thread started and listening...\n";

//     OrderRequest req;

//     while (engineRunning) {
//         // 1. Try to pop an order off the lock-free queue
//         if (orderQueue.pop(req)) {
//             // 2. We got an order! Add it to the OrderBook
//             engine->addOrder(req.id, req.price, req.quantity, req.side);

//             // 3. Run the matcher to see if this new order triggered a trade
//             engine->matchOrder();
//         }
//     }
//     std::cout << "[ENGINE] Shutting down.\n";
// }

// int main() {
//     std::cout << "--- Starting Multithreaded HFT Engine ---\n";

//     OrderBook* engine = new OrderBook();

//     // Spawn the Engine Thread
//     std::thread engineThread(engineThreadLoop, engine);

//     // Give the engine a millisecond to boot up and start listening
//     std::this_thread::sleep_for(std::chrono::milliseconds(1));

//     // Spawn the Network Thread
//     std::thread networkThread(networkThreadLoop);

//     // Wait for the Network Thread to finish sending all its packets
//     networkThread.join();

//     // Let the Engine process the final orders, then shut it down
//     std::this_thread::sleep_for(std::chrono::milliseconds(1));
//     engineRunning = false;
//     engineThread.join();

//     std::cout << "--- System Shutdown Successfully ---\n";

//     delete engine;
//     return 0;
// }