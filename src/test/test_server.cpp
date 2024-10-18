#include "errors.h"
#include "socket.h"
#include "unit_test.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>

static auto test() -> void {
    using dark::assertion;

    auto socket = dark::Socket{
        dark::Socket::Domain::INET4, dark::Socket::Type::STREAM, dark::Socket::Protocol::TCP
    };

    assertion(socket.is_valid(), "socket is invalid");
    assertion(socket.set_opt(socket.opt_reuse), "set_opt failed");
    constexpr auto ip_port = socket.ipv4_port("127.0.0.1", 12345);

    assertion(socket.bind(ip_port), "bind failed");
    assertion(socket.listen(5), "listen failed");

    auto client_sock  = socket.accept().unwrap().first;
    auto buffer       = std::string(24, '\0');
    const auto length = client_sock.recv(buffer).value_or(0);

    std::cout << "Received: " << buffer << '\n';
    std::cout << "Length: " << length << '\n';
}

static auto testcase = Testcase(test);
