#include "error.h"
#include "socket.h"
#include "unit_test.h"
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

static auto test() -> void {
    using dark::assertion;

    auto socket = dark::Socket{
        dark::Socket::Domain::INET4, dark::Socket::Type::STREAM, dark::Socket::Protocol::TCP
    };

    using namespace std::chrono_literals;
    while (!socket.connect(socket.ip_port("127.0.0.1", 12345)))
        std::this_thread::sleep_for(1s);

    auto message = std::string{};
    std::cout << "Enter your message: \n";
    std::getline(std::cin, message);

    auto length = socket.send(message.c_str(), message.size()).value_or(0);
    std::cout << "Sent " << length << " bytes to server" << std::endl;
}

static auto testcase = Testcase(test);
