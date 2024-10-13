#include "error.h"
#include "socket.h"
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    using dark::assertion;

    auto socket = dark::Socket{
        dark::Socket::Domain::INET4, dark::Socket::Type::STREAM, dark::Socket::Protocol::TCP
    };

    assertion(socket.connect(socket.ip_port("127.0.0.1", 12345)), "Failed to connect to server");

    auto message = std::string{};
    std::cout << "Enter message: ";
    std::getline(std::cin, message);

    auto length = socket.send(message.c_str(), message.size()).value_or(0);
    std::cout << "Sent " << length << " bytes to server" << std::endl;
}
