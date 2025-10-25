#include "metrics.h"
#include <iomanip>
#include <sstream>

std::string Metrics::to_json() const {
    /**
     * Serialize metrics to JSON string
     */
    std::ostringstream ss;
    ss << "{";
    ss << "\"tcp_accepts\":" << tcp_accepts.load() << ",";
    ss << "\"tcp_closes\":" << tcp_closes.load() << ",";
    ss << "\"tcp_rx_bytes\":" << tcp_rx_bytes.load() << ",";
    ss << "\"tcp_tx_bytes\":" << tcp_tx_bytes.load() << ",";
    ss << "\"tcp_active\":" << tcp_active.load() << ",";
    ss << "\"udp_rx_packets\":" << udp_rx_packets.load() << ",";
    ss << "\"udp_tx_packets\":" << udp_tx_packets.load() << ",";
    ss << "\"udp_rx_bytes\":" << udp_rx_bytes.load() << ",";
    ss << "\"udp_tx_bytes\":" << udp_tx_bytes.load();
    ss << "}";
    return ss.str();
}