# Raptor OME: Ultra-Low Latency C++ Matching Engine

A sub-microsecond, multi-threaded Limit Order Book (LOB) and trade matching engine engineered in C++17 for High-Frequency Trading (HFT) environments.

**Tech Stack :** Modern C++ (C++17), SPSC Lock-Free Ring Buffer, `std::atomic`, Memory Barriers (`acquire`/`release`), Custom Allocators, POSIX Threads.

## 🏗️ Core Architecture

* **Deterministic O(1) Order Book:** Bypasses `std::map` tree-traversal by using a statically allocated flat array where the index maps directly to the price tick. Price-level queues use an intrusive doubly-linked list to eliminate `std::list` memory scatter.
* **Zero-Allocation Hot Path:** Bypasses OS heap management (`new`/`delete`) during live trading. Uses a custom pure O(1) pointer-stack memory pool pre-allocated at startup to recycle L1 cache lines without memory fragmentation.
* **Hardware-Aware Concurrency:** Decouples network ingestion and order matching via a lock-free Single-Producer, Single-Consumer (SPSC) ring buffer. Mitigates false-sharing cache invalidation by aligning atomic `head` and `tail` indices to 64-byte boundaries (`alignas(64)`).

## ⏱️ Nanosecond Benchmarks

End-to-end latency (network ingestion to LOB update), excluding initial cache-warming:

    (base) <username>@MacBook-Air projectcpp % g++ -O3 -std=c++17 main.cpp OrderBook.cpp -pthread -o raptor_ome
    (base) <username>@MacBook-Air projectcpp % ./raptor_ome 
    --- Starting Multithreaded HFT Engine ---
    [ENGINE] Thread started and listening...
    [NETWORK] Thread started on CPU core.
    [METRIC] Order 100 Processed. End-to-End Latency: 6667 ns
    [METRIC] Order 101 Processed. End-to-End Latency: 292 ns
    [METRIC] Order 102 Processed. End-to-End Latency: 583 ns
    [METRIC] Order 103 Processed. End-to-End Latency: 791 ns
    [NETWORK] All orders sent.
    
    ========================================
    [BENCHMARK SUMMARY]
    Total Orders Processed: 4
    Total Time Taken: 8333 ns
    Average Latency: 2083 ns
    ========================================
    [ENGINE] Shutting down.
    --- System Shutdown Successfully ---

## 🛠️ Build & Run

Requires GCC, C++17 support, and the pthread library.

    git clone https://github.com/yourusername/raptor-ome.git
    cd raptor-ome
    g++ -O3 -std=c++17 main.cpp OrderBook.cpp -pthread -o raptor_ome
    ./raptor_ome
