#include "hw1/forward.h"
#include "socket.h"
#include <atomic>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <optional>
#include <shared_mutex>
#include <string_view>
#include <thread>
#include <unordered_map>

static std::shared_mutex cache_mutex;
static std::unordered_map<std::string, std::string> cache;
static std::atomic_size_t counter{};

static auto look_up_cache(const std::string &str) -> std::optional<std::string> {
    std::shared_lock lock{cache_mutex};
    if (auto iter = cache.find(str); iter != cache.end()) {
        return iter->second;
    } else {
        return {};
    }
}

static auto push_to_cache(std::string host, std::string response) -> void {
    std::unique_lock lock{cache_mutex};
    cache.try_emplace(std::move(host), std::move(response));
}

static auto make_connection_impl(dark::Socket client) {
    const auto uid = counter++;
    std::cout << std::format("[{}] New connection\n", uid);

    auto buffer = std::string(4096, '\0');
    client.recv(buffer).unwrap("recv failed");

    // Find the host in the request like "Host: www.google.com\r\n"
    auto request = std::string_view{buffer};
    request      = request.substr(request.find("Host: ") + 6);
    request      = request.substr(0, request.find("\r\n"));
    auto host    = std::string{request};

    // Check if the response is cached
    if (auto cached = look_up_cache(host); cached) {
        std::cout << std::format("[{}] Cache hit!\n", uid);
        client.send(*cached).unwrap("send failed");
        return;
    }

    // connect to the target host
    std::cout << std::format("[{}] Connecting to {}\n", uid, host);
    auto target = dark::Socket{dark::Domain::INET4, dark::Type::STREAM, dark::Protocol::TCP};
    target.connect(dark::Address{host, "http", 80}).unwrap();
    target.send(buffer).unwrap("send failed: {}");

    buffer.clear();
    // Keep reading until the connection is closed
    auto content_length = std::size_t{};
    auto header_length  = std::size_t{};

    while (true) {
        auto response = std::string(4096, '\0');
        target.recv(response).unwrap("recv failed: {}");
        buffer += response;
        if (header_length == 0) {
            auto header = std::string_view{buffer};
            auto pos    = header.find("\r\n\r\n");
            if (pos == std::string_view::npos)
                continue; // Header is not complete yet
            // Header is complete
            header_length    = pos + 4;
            auto header_str  = header.substr(0, header_length);
            auto content_str = header.substr(header_str.find("Content-Length: "));
            content_str      = content_str.substr(0, content_str.find("\r\n"));
            content_length   = std::stoul(std::string{content_str.substr(16)});
        }

        if (buffer.size() >= header_length + content_length)
            break;
    }

    std::cout << std::format("[{}] Received {} bytes\n", uid, buffer.size());
    push_to_cache(std::move(host), buffer);
    client.send(buffer).unwrap("send failed");

    // close the connection
    std::cout << std::format("[{}] Connection closed\n", uid);
}

static auto make_connection(dark::Socket client) -> void {
    try {
        make_connection_impl(std::move(client));
    } catch (const std::exception &e) { std::cerr << "Error: " << e.what() << '\n'; }
}

static auto interrupt_handler(int) -> void {
    std::cout << "\nProxy server is shutting down\n";
    std::exit(0);
}

auto run_proxy(std::string_view ip, std::uint16_t port) -> void {
    static_cast<void>(std::signal(SIGINT, interrupt_handler));
    auto server = dark::Socket{dark::Domain::INET4, dark::Type::STREAM, dark::Protocol::TCP};

    server.set_opt(server.opt_reuse).unwrap();
    server.bind(dark::Address{ip, port}).unwrap();
    server.listen(10).unwrap();

    while (auto conn = server.accept()) {
        std::cout << "Proxy connection accepted\n";
        std::thread{make_connection, std::move(conn.unwrap().first)}.detach();
    }

    std::cout << "Proxy server is shutting down\n";
}
