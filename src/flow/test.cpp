// server: send an extremly large file (100G) for example.
// client: receive a large file and watch the bandwidth.
#include "address.h"
#include "socket.h"
#include <chrono>
#include <format>
#include <iostream>
#include <string>

static auto server() -> void {
    auto data   = std::string(1024 * 1024 * 1024, 'a');
    auto socket = dark::Socket{dark::Domain::INET4, dark::Type::STREAM, dark::Protocol::TCP};

    socket.set_opt(socket.opt_reuse).unwrap();
    socket.bind(dark::Address{"127.0.0.1", "6789"}).unwrap();

    socket.listen(5).unwrap();

    auto client = socket.accept().unwrap().first;
    for (int i = 0; i < 100; ++i)
        client.send(data).unwrap();
}

static auto client() -> void {
    auto buffer = std::string(1024 * 1024, 'a');
    auto socket = dark::Socket{dark::Domain::INET4, dark::Type::STREAM, dark::Protocol::TCP};

    socket.connect(dark::Address{"127.0.0.1", "6789"}).unwrap();

    while (true) {
        auto tic = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 1024; ++i)
            socket.recv(buffer).unwrap();
        auto toc   = std::chrono::high_resolution_clock::now();
        auto dur   = std::chrono::duration_cast<std::chrono::milliseconds>(toc - tic).count();
        auto speed = 1024.0 * 1024 * 1024 / dur;
        std::cout << std::format("speed: {:.2f} MB/s\n", speed);
    }
}

auto main(int argc, const char **argv) -> int {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " server|client\n";
        return 1;
    }

    if (std::string{argv[1]} == "server")
        server();
    else if (std::string{argv[1]} == "client")
        client();
    else
        std::cerr << "Usage: " << argv[0] << " server|client\n";

    return 0;
}
