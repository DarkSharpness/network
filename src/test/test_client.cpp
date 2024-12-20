#include "address.h"
#include "errors.h"
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

    auto socket = dark::Socket{dark::Domain::INET4, dark::Type::STREAM, dark::Protocol::TCP};

    using namespace std::chrono_literals;
    while (!socket.connect(dark::Address{"127.0.0.1", 12345}))
        std::this_thread::sleep_for(1s);

    auto length = socket.send("Hello world!").value_or(0);
    std::cout << "Sent " << length << " bytes to server" << std::endl;
}

static auto testcase = Testcase(test);
