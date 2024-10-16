#include "error.h"
#include "socket.h"
#include "unit_test.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
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

    auto client_sock = socket.accept().value().first;
    char buf[24];
    const auto length = client_sock.recv(buf).value_or(0);
    const auto str    = std::string_view{buf, length};

    std::cout << "Received: " << str << '\n';
    std::cout << "Length: " << length << '\n';
}

static auto testcase = Testcase(test);
