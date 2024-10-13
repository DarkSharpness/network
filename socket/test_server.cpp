#include "error.h"
#include "socket.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    using dark::assertion;

    auto socket = dark::Socket{
        dark::Socket::Domain::INET4, dark::Socket::Type::STREAM, dark::Socket::Protocol::TCP
    };

    assertion(socket.bind(socket.ip_port("127.0.0.1", 12345)), "bind failed");
    assertion(socket.listen(5), "listen failed");

    while (true) {
        auto client_sock = socket.accept().value();
        char buf[24];
        const auto length = client_sock.receive(buf, sizeof(buf)).value_or(0);
        const auto str    = std::string_view{buf, length};

        std::cout << "Received: " << str << '\n';
        std::cout << "Length: " << length << '\n';
    }
}
