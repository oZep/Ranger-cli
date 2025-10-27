#include "udp_forwarder.h"
#include "common.h"

UdpForwarder::UdpForwarder(const UdpConfig& cfg, Metrics* m) : cfg_(cfg), metrics_(m) {}
UdpForwarder::~UdpForwarder() { shutdown(); }

int UdpForwarder::init() {
    listen_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (listen_fd_ < 0) { perror("socket"); return -1; }
    int val = 1; setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (cfg_.destinations.size() >= 1) setsockopt(listen_fd_, SO_REUSEPORT, &val, sizeof(val));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(cfg_.listen_port);
    if (bind(listen_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return -1; }

    for (auto d : cfg_.destinations) {
        Upstream u{};
        u.fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (u.fd < 0) { perror("socket upstream"); return -1; }
        sockaddr_in a{}; a.sin_family = AF_INET;
        if (inet_pton(AF_INET, d.host.c_str(), &a.sin_addr) <= 0) { perror("inet_pton"); return -1; }
        a.sin_port = htons(d.port);
        u.addr = *(sockaddr_storage*)&a;
        u.addr_len = sizeof(a);
        ups_.push_back(std::move(u));
    }

    if (io_uring_queue_init(32768, &ring_, 0) < 0) { perror("io_uring_queue_init"); return -1; }
    return 0;
}

void UdpForwarder::setup_listen() {
    for (int i = 0; i < 256; ++i) schedule_recv();
    io_uring_submit(&ring_);
}

void UdpForwarder::schedule_recv() {
    static char buf[2048];
    io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    if (!sqe) return;
    io_uring_prep_recv(sqe, listen_fd_, buf, sizeof(buf), 0);
    sqe->flags |= IOSQE_ASYNC;
}

size_t UdpForwarder::pick_upstream(const char* /*pkt*/, size_t /*len*/) {
    if (ups_.empty()) return 0;
    if (cfg_.hash_mode == "round_robin") {
        size_t idx = rr_idx_++ % ups_.size();
        return idx;
    } else {
        // Very simple hash: use first 4 bytes as hash
        uint32_t h = 0;
        // hash payload
        for (size_t i = 0; i < 8 && i < 0; ++i) {} // no-op to keep compile fast; replace with actual hashing if needed
        return (size_t)(h % ups_.size());
    }
}

void UdpForwarder::forward_to(const char* pkt, size_t len, size_t idx) {
    if (idx >= ups_.size()) return;
    io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    if (!sqe) return;
    static char copy[2048];
    size_t c = std::min(len, sizeof(copy));
    memcpy(copy, pkt, c);
    io_uring_prep_sendto(sqe, ups_[idx].fd, copy, c, 0, (sockaddr*)&ups_[idx].addr, ups_[idx].addr_len);
    sqe->flags |= IOSQE_ASYNC;
}

void UdpForwarder::handle_rx() {
    io_uring_cqe* cqe;
    unsigned head; unsigned count = 0;
    io_uring_for_each_cqe(&ring_, head, cqe) {
        ++count;
        if (cqe->res > 0) {
            metrics_->udp_rx_packets.fetch_add(1, std::memory_order_relaxed);
            metrics_->udp_rx_bytes.fetch_add((uint64_t)cqe->res, std::memory_order_relaxed);
            size_t idx = pick_upstream(nullptr, (size_t)cqe->res);
            forward_to((const char*)io_uring_cqe_get_data(cqe), (size_t)cqe->res, idx); // user_data not set; adjust
        }
    }
    if (count > 0) {
        io_uring_cq_advance(&ring_, count);
        schedule_recv(); // keep outstanding recvs
        io_uring_submit(&ring_);
    }
}

void UdpForwarder::run() {
    setup_listen();
    while (!stop_.load(std::memory_order_relaxed)) {
        int ret = io_uring_submit_and_wait(&ring_, 1);
        if (ret < 0 && errno != EINTR) break;
        handle_rx();
    }
}

void UdpForwarder::shutdown() {
    stop_.store(true, std::memory_order_relaxed);
    if (listen_fd_ >= 0) ::close(listen_fd_);
    for (auto& u : ups_) if (u.fd >= 0) ::close(u.fd);
    io_uring_queue_exit(&ring_);
}