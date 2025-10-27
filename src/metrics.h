#pragma once
#include <atomic> // https://www.geeksforgeeks.org/cpp/cpp-11-atomic-header/
#include <cstdint>
#include <string>
#include <chrono>

struct Metrics {
    std::atomic<uint64_t> tcp_accepts{0};
    std::atomic<uint64_t> tcp_closes{0};
    std::atomic<uint64_t> tcp_rx_bytes{0};
    std::atomic<uint64_t> tcp_tx_bytes{0};
    std::atomic<uint64_t> tcp_active{0};
    std::atomic<uint64_t> udp_rx_packets{0};
    std::atomic<uint64_t> udp_tx_packets{0};
    std::atomic<uint64_t> udp_rx_bytes{0};
    std::atomic<uint64_t> udp_tx_bytes{0};

    std::string to_json() const;
};