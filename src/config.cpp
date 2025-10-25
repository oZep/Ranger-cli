#include "config.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp> // https://github.com/nlohmann/json 
using json = nlohmann::json;

static void parse_json(const std::string& path, Config& cfg) {

    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "Failed to open config: " << path << "\n";
        return;
    }

    json j; f >> j;
    if (j.contains("tcp")) {
        auto t = j["tcp"];
        cfg.tcp.enabled = t.value("enabled", cfg.tcp.enabled);
        cfg.tcp.port = t.value("port", cfg.tcp.port);
        cfg.tcp.mode = t.value("mode", cfg.tcp.mode);
        cfg.tcp.upstream_host = t.value("upstream_host", cfg.tcp.upstream_host);
        cfg.tcp.upstream_port = t.value("upstream_port", cfg.tcp.upstream_port);
        cfg.tcp.tls_pass_through = t.value("tls_pass_through", cfg.tcp.tls_pass_through);
        cfg.tcp.cert_file = t.value("cert_file", cfg.tcp.cert_file);
        cfg.tcp.key_file = t.value("key_file", cfg.tcp.key_file);
    }

    if (j.contains("udp")) {
        auto u = j["udp"];
        cfg.udp.enabled = u.value("enabled", cfg.udp.enabled);
        cfg.udp.listen_port = u.value("listen_port", cfg.udp.listen_port);
        cfg.udp.hash_mode = u.value("hash_mode", cfg.udp.hash_mode);
        if (u.contains("destinations")) {
            for (auto d : u["destinations"]) {
                cfg.udp.destinations.push_back({d["host"], (uint16_t)d["port"]});
            }
        }
    }
    if (j.contains("reuseport")) cfg.reuseport = j.value("reuseport", cfg.reuseport);
    if (j.contains("conn_limit")) cfg.conn_limit = j.value("conn_limit", cfg.conn_limit);
}

Config load_config(int argc, char** argv) {
    Config cfg;
    cfg.udp.destinations = {{"127.0.0.1", 7001}};
    if (argc > 1) {
        parse_json(argv[1], cfg);
    } else {
        std::cerr << "Usage: ./edgehub config.json\n";
    }
    return cfg;
}
