#include "socket.h"
#include "hw1/forward.h"
#include <cstdint>
#include <error.h>
#include <format>
#include <iostream>
#include <netinet/in.h>
#include <string_view>
#include <vector>

auto run_proxy(std::string_view ip, std::uint16_t port) -> void{
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
        auto [client, _] = std::move(*conn);
        auto buffer = std::vector<char>(4096);
        auto length = client.recv(buffer);
        assertion(length, "recv failed");
        std::cout << std::format("Recevied {} bytes\n", *length);
        // get the first line of the request and find target host
        auto request = std::string_view(buffer.data(), *length);
        auto host = request.substr(request.find("Host: ") + 6);
        host = host.substr(0, host.find("\r\n"));
        // connect to the target host
        auto target = dark::Socket{
            dark::Socket::Domain::INET4, dark::Socket::Type::STREAM, dark::Socket::Protocol::TCP
        };
        // split as host and port
        host = host.substr(0, host.find(':'));
        std::cout << std::format("Host: {}\n", host);
        auto target_addr = target.link_to_ipv4(std::string(host));
        assertion(target_addr, "dns lookup failed");
        target_addr->sin_port = ::htons(80);
        assertion(target.connect(*target_addr), "connect failed");

        // start forwarding
        auto view = std::string_view(buffer.data(), *length);
        assertion(target.send(view), "send failed");

        buffer.resize(1024 * 1024);
        length = target.recv(buffer);
        assertion(length, "recv failed");
        std::cout << std::format("Recevied {} bytes\n", *length);

        // forward the response to the client
        view = std::string_view(buffer.data(), *length);
        assertion(client.send(view), "send failed");

        // close the connection
        std::cout << "Connection closed\n";
    }
    std::cout << "Connection closed\n";
}