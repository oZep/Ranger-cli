#pragma once

// forward-declare config type so the header is safe regardless of include order
struct UdpConfig;

#include "config.h"
#include "metrics.h"
#include <liburing.h>
#include <cstdint>
#include <vector>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

struct UdpConfig;

class UdpForwarder {
    public:
        UdpForwarder(const UdpConfig& cfg, Metrics* m);
        ~UdpForwarder();
        int init();
        void run();
        void shutdown();

    private:
        const UdpConfig& cfg_;
        Metrics* metrics_;
        io_uring ring_;
        std::atomic<bool> stop_{false};

        int listen_fd_ = -1;
        struct Upstream {
            int fd;
            sockaddr_storage addr;
            socklen_t addr_len;
        };
        std::vector<Upstream> ups_;
        size_t rr_idx_ = 0;

        void setup_listen();
        void schedule_recv();
        void handle_rx();
        size_t pick_upstream(const char* pkt, size_t len);
        void forward_to(const char* pkt, size_t len, size_t idx);
};