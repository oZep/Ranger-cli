#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct TcpConfig {
    bool enabled = true;
    uint16_t port = 9000;
    std::string mode = "echo"; // or proxy
    std::string upstream_host = "127.0.0.1";
    uint16_t upstream_port = 9001;
    bool tls_pass_through = false;
    std::string cert_file;
    std::string key_file;
};

struct UdpDest {
    std::string host;
    uint16_t port;
};

struct UdpConfig {
    bool enabled = true;
    uint16_t listen_port = 7000;
    std::vector<UdpDest> destinations; // multiple endpoints to spread traffic
    std::string hash_mode = "round_robin"; // or hash
};

struct Config {
    TcpConfig tcp;
    UdpConfig udp;
    bool reuseport = true;
    int conn_limit = 100000;
};

Config load_config(int argc, char** argv);