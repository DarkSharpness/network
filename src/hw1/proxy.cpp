#include "hw1/forward.h"
#include "socket.h"
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <error.h>
#include <format>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <shared_mutex>
#include <string_view>
#include <thread>
#include <unordered_map>

static std::shared_mutex cache_mutex;
static std::unordered_map<std::string, std::string> cache;

static auto make_connection(dark::Socket client) {
    using dark::assertion;
    assertion(client, "accept failed");

    auto buffer = std::string(4096, '\0');
    auto length = client.recv(buffer);
    assertion(length, "recv failed");
    std::cout << std::format("Recevied {} bytes\n", *length);

    // get the first line of the request and find target host
    auto request = std::string_view(buffer.data(), *length);
    auto host    = request.substr(request.find("Host: ") + 6);
    host         = host.substr(0, host.find("\r\n"));
    // Acquire read lock
    std::shared_lock lock{cache_mutex};

    if (auto iter = cache.find(std::string(host)); iter != cache.end()) {
        // return cached response
        std::cout << "Cache hit! Returning cached response\n";
        assertion(client.send(iter->second), "send failed");
        return;
    }
    lock.unlock();

    // connect to the target host
    auto target = dark::Socket{
        dark::Socket::Domain::INET4, dark::Socket::Type::STREAM, dark::Socket::Protocol::TCP
    };

    // split as host and port
    auto host_str = std::string(host);
    std::cout << std::format("Host: {}\n", host_str);
    auto target_addr = target.link_to_ipv4(host_str);
    assertion(target_addr, "dns lookup failed");
    target_addr->sin_port = htons(80);
    assertion(target.connect(*target_addr), "connect failed");

    // start forwarding
    auto view = std::string_view(buffer.data(), *length);
    assertion(target.send(view), "send failed");

    length = target.recv(buffer);
    assertion(length, "recv failed");
    std::cout << std::format("Recevied {} bytes\n", *length);

    buffer.resize(*length);
    assertion(client.send(buffer), "send failed");

    // add to cache
    std::unique_lock ulock{cache_mutex};
    cache[host_str] = std::move(buffer);
    ulock.unlock();

    // close the connection
    std::cout << "Connection closed\n";
}

static auto interrupt_handler(int) -> void {
    std::cout << "\nProxy server is shutting down\n";
    std::exit(0);
}

auto run_proxy(std::string_view ip, std::uint16_t port) -> void {
    static_cast<void>(std::signal(SIGINT, interrupt_handler));

    auto server = dark::Socket{
        dark::Socket::Domain::INET4, dark::Socket::Type::STREAM, dark::Socket::Protocol::TCP
    };

    const auto server_addr = server.ipv4_port(ip, port);
    using dark::assertion;
    assertion(server.set_opt(server.opt_reuse), "set_opt failed");
    assertion(server.bind(server_addr), "bind failed");
    // Maximum number of pending connections
    assertion(server.listen(10), "listen failed");

    while (auto conn = server.accept()) {
        std::cout << "Connection accepted\n";
        std::thread{make_connection, std::move(conn->first)}.detach();
    }

    std::cout << "Connection closed\n";
}
