## Ranger-cli

####

Long running C++ backend (packet_server) captures with libcap and accepts control commands (set_filter, pause, send) over the existing control socket (TCP 127.0.0.1:9001). The Go TUI sends a JSON command when the user types a port; the C++ backend applies a BPF filter (tcp AND port XXXX) and starts sending packet events to the UI.

####

A multi-protocol server with:
- Async TCP server (io_uring) with zero-copy in-kernel send where supported
- UDP packet forwarder with load distribution and hashing
- Observability (counters, latency histograms)

Build/run on Linux with libcap and OpenSSL

## Architecture & Roadmap

This project is intended to become a terminal-first packet inspector (think "Wireshark in the terminal"). The design splits responsibilities so C++ handles capture, parsing, and packet I/O while a Go TUI (Bubble Tea + Lip Gloss) provides a polished, scrollable interface.

High-level architecture
- Capture & logic (C++):
	- Use libpcap (or existing capture) to obtain raw packets, parse layers (Ethernet/IP/TCP/UDP), and produce lightweight JSON events.
	- Expose an IPC endpoint for the UI (prototype: TCP 127.0.0.1:9001; later: Unix domain socket).
	- Accept control commands from the UI (send packet, pause/resume, set filter) and respond with ack/error.

- UI & interaction (Go + Bubble Tea/Lip Gloss):
	- Connect to the IPC endpoint and stream incoming events.
	- Maintain an in-memory ring buffer of recent packets.
	- Provide list view (scrollable) and details view (hex/raw payload) and a modal for sending packets.

Message schema (newline-delimited JSON)
- Packet event (server -> UI):
```
{ "type":"packet", "id":123, "dir":"in", "ts":1698475200000,
	"network": {"src":"1.2.3.4","dst":"5.6.7.8","proto":"tcp"},
	"transport": {"sport":1234,"dport":80},
	"len":42, "payload_hex":"48656c6c6f" }
```

- Command (UI -> server):
```
{ "type":"command", "cmd":"send", "proto":"udp", "host":"127.0.0.1", "port":7001, "payload_base64":"aGVsbG8=" }
```
