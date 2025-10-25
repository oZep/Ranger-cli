## Ranger-cli


A multi-protocol server with:
    Async TCP server (io_uring) with zero-copy in-kernel send where supported
    UDP packet forwarder with load distribution and hashing
    Clean, testable C++ architecture
    Observability (counters, latency histograms)
    Optional TLS, optional eBPF/XDP path, optional QUIC/HTTP/3 (small step-by-step additions)
Build/run on Linux with liburing and OpenSSL (for optional TLS)
