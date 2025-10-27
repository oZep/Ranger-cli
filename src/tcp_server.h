#pragma once

// forward-declare config type so the header is safe regardless of include order
struct TcpConfig;

#include "config.h"
#include "metrics.h"
#include <liburing.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

struct FixedBuf {
    static constexpr size_t SZ = 64 * 1024;
    char data[SZ];
    size_t len = 0;
};

struct Connection {
    int fd = -1;
    FixedBuf rb;           // per-connection read buffer
    bool readable = true;  // to gate read events
};

class TcpServer {
    public:
        TcpServer(const TcpConfig& cfg, Metrics* m);
        ~TcpServer();
        int init();
        void run();
        void shutdown();

    private:
        const TcpConfig& cfg_;
        Metrics* metrics_;
        int listen_fd_ = -1;
        io_uring ring_;
        std::atomic<bool> stop_{false};

        // TLS (pass-through)
        SSL_CTX* ssl_ctx_ = nullptr;

        void setup_listen();
        void accept_connections();
        void process_read(int fd);
        void schedule_read(Connection* c);
        void schedule_write(int fd, const char* buf, size_t len);
        void close_fd(int& fd);

        // TLS helpers (optional)
        bool init_tls();
        void teardown_tls();
        bool ssl_handshake(int client_fd);
        size_t ssl_read(int fd, char* out, size_t cap);
        size_t ssl_write(int fd, const char* data, size_t len);
};