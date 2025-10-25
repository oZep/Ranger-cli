## Ranger-cli

A multi-protocol server with:
- Async TCP server (io_uring) with zero-copy in-kernel send where supported
- UDP packet forwarder with load distribution and hashing
- Observability (counters, latency histograms)

Build/run on Linux with liburing and OpenSSL