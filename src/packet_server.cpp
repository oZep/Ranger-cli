#include <arpa/inet.h>
#include <chrono>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

// Simple TCP server that streams newline-delimited JSON "packet" events
// to a single connected client and accepts newline-delimited JSON commands
// from the client. This is a prototype: the places where real packet
// capture or command handling should happen are marked with TODO comments
// so you can implement them as an exercise.

static std::string make_fake_packet_json(int id) {
    // TODO: Replace this function with actual packet-capture formatting.
    // For the exercise: implement a richer packet structure (inspect
    // real packets, include raw bytes encoded as base64, etc.).
    using namespace std::chrono;
    auto ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    std::ostringstream ss;
    ss << "{"
       << "\"type\":\"packet\",";
    ss << "\"id\":" << id << ",";
    ss << "\"dir\":\"in\",";
    ss << "\"ts\":" << ms << ",";
    ss << "\"src\":\"127.0.0.1:12345\",";
    ss << "\"dst\":\"127.0.0.1:9000\",";
    ss << "\"len\":12,";
    ss << "\"payload\":\"hello-world\"";
    ss << "}\n";
    return ss.str();
}

static void handle_command(const std::string &cmd_json, int client_fd) {
    // TODO: Parse incoming JSON command and perform the requested action
    // (e.g., send a UDP/TCP packet, change capture filters, etc.). For the
    // exercise: implement proper JSON parsing and perform a real send.

    // For now we just echo an ack back to the client so the UI can show a response.
    std::string ack = "{\"type\":\"ack\",\"status\":\"ok\",\"raw\":\"";
    // Make sure to escape quotes/simple characters if you include cmd_json
    for (char c : cmd_json) {
        if (c == '"') ack += '\\"';
        else if (c == '\n') ack += "\\n";
        else ack.push_back(c);
    }
    ack += "}\n";
    send(client_fd, ack.c_str(), (int)ack.size(), 0);
}

int main(int argc, char **argv) {
    const char *bind_addr = "127.0.0.1";
    const int port = 9001;

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        std::cerr << "socket() failed: " << strerror(errno) << std::endl;
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, bind_addr, &addr.sin_addr);

    if (bind(listen_fd, (sockaddr *)&addr, sizeof(addr)) != 0) {
        std::cerr << "bind() failed: " << strerror(errno) << std::endl;
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, 1) != 0) {
        std::cerr << "listen() failed: " << strerror(errno) << std::endl;
        close(listen_fd);
        return 1;
    }

    std::cout << "packet_server: listening on " << bind_addr << ":" << port << std::endl;

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(listen_fd, (sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            std::cerr << "accept() failed: " << strerror(errno) << std::endl;
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        std::cout << "client connected: " << client_ip << ":" << ntohs(client_addr.sin_port) << std::endl;

        // Launch a thread to send fake packet events periodically.
        std::thread sender([client_fd]() {
            int id = 1;
            while (true) {
                std::string pkt = make_fake_packet_json(id++);
                ssize_t n = send(client_fd, pkt.c_str(), (int)pkt.size(), 0);
                if (n <= 0) {
                    // client disconnected or error
                    break;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            // close will be performed in main thread when cleanup occurs
        });

        // Read newline-delimited commands from the client
        std::string buffer;
        char tmp[512];
        while (true) {
            ssize_t r = recv(client_fd, tmp, sizeof(tmp), 0);
            if (r <= 0) break; // disconnected or error
            buffer.append(tmp, tmp + r);
            // find newline
            size_t pos;
            while ((pos = buffer.find('\n')) != std::string::npos) {
                std::string line = buffer.substr(0, pos);
                if (!line.empty()) {
                    // handle the command in-place (could be dispatched to worker)
                    handle_command(line, client_fd);
                }
                buffer.erase(0, pos + 1);
            }
        }

        std::cout << "client disconnected" << std::endl;
        close(client_fd);
        // allow sender thread to exit
        if (sender.joinable()) sender.join();
    }

    close(listen_fd);
    return 0;
}
