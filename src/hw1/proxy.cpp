#include "errors.h"
#include "hw1/cache.h"
#include "hw1/forward.h"
#include "hw1/html.h"
#include "select.h"
#include "socket.h"
#include <atomic>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <iostream>
#include <netinet/in.h>
#include <optional>
#include <string>
#include <string_view>
#include <sys/select.h>
#include <thread>
#include <utility>

static std::atomic_size_t counter{};

static auto forward_data(dark::Socket &client, dark::Socket &target) -> std::string {
    auto buffer = std::string(4096, '\0');
    auto reply  = std::string{};
    try {
        while (true) {
            auto result = dark::select({client, target}, nullptr, nullptr).unwrap();
            static_assert(sizeof(result) == sizeof(fd_set));
            if (result.reads.contains(client)) {
                client.recv(buffer).unwrap();
                if (buffer.size() == 0)
                    break;
                target.send(buffer).unwrap();
            }
            if (result.reads.contains(target)) {
                target.recv(buffer).unwrap();
                if (buffer.size() == 0)
                    break;
                client.send(buffer).unwrap();
                reply += buffer;
            }
        }
    } catch (const std::exception &) {}

    return reply;
}

static auto make_connection_impl(dark::Socket client) -> void {
    const auto uid = counter++;
    std::cout << std::format("[{}] New connection\n", uid);

    auto buffer = std::string(4096, '\0');

    // Receive the request from the client
    const auto message = receive_http(client, buffer);
    const auto method  = parse_http(message, "", " ");
    const auto host    = std::string{parse_http(message, " ", " ")};
    std::cout << std::format("[{}] Connection to {}\n", uid, host);
    const auto host_info = parse_host(host);
    dark::assertion(host_info, "Invalid host: {}", host);
    const auto [addr, is_http] = *host_info;
    const auto is_http_get     = method == "GET" && is_http;

    // if http 'GET' && http request
    if (is_http_get) {
        // Check if the response is cached
        if (auto cached = look_up_cache(host)) {
            std::cout << std::format("[{}] Cache hit!\n", uid);
            client.send(*cached).unwrap();
            return;
        }
    }

    auto target = dark::Socket{dark::Domain::INET4, dark::Type::STREAM, dark::Protocol::TCP};
    target.connect(addr).unwrap();

    if (method == "CONNECT") {
        client.send("HTTP/1.1 200 OK\r\n\r\n").unwrap();
    } else {
        target.send(message).unwrap();
    }

    auto reply = forward_data(client, target);
    if (is_http_get) {
        std::cout << std::format("[{}] Caching response\n", uid);
        push_to_cache(host, std::move(reply));
    }
    std::cout << std::format("[{}] Connection closed\n", uid);
}

static auto make_connection(dark::Socket client) -> void {
    try {
        make_connection_impl(std::move(client));
    } catch (const std::exception &e) { std::cerr << "Error: " << e.what() << '\n'; }
}

inline auto interrupt_handler(int) -> void {
    std::cout << "\nProxy server is shutting down\n";
    save_cache_to_file();
    std::exit(0);
}

auto run_proxy(std::string_view ip, std::uint16_t port) -> void {
    static_cast<void>(std::signal(SIGINT, interrupt_handler));
    load_cache_from_file();

    auto server = dark::Socket{dark::Domain::INET4, dark::Type::STREAM, dark::Protocol::TCP};

    server.set_opt(server.opt_reuse).unwrap();
    server.bind(dark::Address{ip, port}).unwrap();
    server.listen(10).unwrap();

    std::cout << "Proxy is ready to serve.\n";
    while (auto conn = server.accept()) {
        std::cout << "Proxy connection accepted\n";
        std::thread{make_connection, std::move(conn.unwrap().first)}.detach();
    }

    std::cout << "Proxy server is shutting down\n";
}
