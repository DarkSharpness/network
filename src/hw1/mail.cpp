#include "hw1/forward.h"
#include "socket.h"
#include <array>
#include "errors.h"
#include <format>
#include <iostream>
#include <netdb.h>
#include <string_view>
#include <sys/socket.h>

auto send_mail(std::string_view msg, std::string_view sender, std::string_view target) -> void {
    const auto mail_server = "mail.sjtu.edu.cn";
    const auto mail_port   = 25;
    using dark::assertion;

    auto ip_port = dark::Socket::link_to_ipv4(mail_server).unwrap("fail to resolve mail server");
    ip_port.sin_port = ::htons(mail_port);

    auto client = dark::Socket{
        dark::Socket::Domain::INET4, dark::Socket::Type::STREAM, dark::Socket::Protocol::TCP
    };
    assertion(client.connect(ip_port), "fail to connect to mail server");

    // accept a welcome message
    std::array<char, 1024> buffer = {};
    client.recv(buffer).unwrap("fail to receive welcome message");

    std::cout << "Received: " << buffer.data() << std::endl;
    buffer.fill(0);

    // send HELO
    client.send("HELO DarkSharpness\r\n").unwrap("fail to send HELO");
    client.recv(buffer).unwrap("fail to receive HELO response");

    if (std::string_view{buffer.data(), 3} != "250") {
        // Retry with HELO
        client.send("HELO DarkSharpness\r\n").unwrap("fail to send HELO");
        client.recv(buffer).unwrap("fail to receive HELO response");
    }

    std::cout << "Received: " << buffer.data() << std::endl;
    buffer.fill(0);
    std::cout << "Now sending mail..." << std::endl;

    // send MAIL FROM
    client.send(std::format("MAIL FROM: <{}>\r\n", sender)).unwrap("fail to send MAIL FROM");
    client.recv(buffer).unwrap("fail to receive MAIL FROM response");
    std::cout << "Received: " << buffer.data() << std::endl;
    buffer.fill(0);

    // send RCPT TO
    client.send(std::format("RCPT TO: <{}>\r\n", target)).unwrap("fail to send RCPT TO");
    client.recv(buffer).unwrap("fail to receive RCPT TO response");
    std::cout << "Received: " << buffer.data() << std::endl;
    buffer.fill(0);

    // send DATA
    client.send("DATA\r\n").unwrap("fail to send DATA");
    client.recv(buffer).unwrap("fail to receive DATA response");
    std::cout << "Received: " << buffer.data() << std::endl;
    buffer.fill(0);

    // send message
    auto message = std::format(
        "From: <{}>\r\n"
        "To: <{}>\r\n"
        "Subject: Hello world!\r\n"
        "\r\n"
        "{}\r\n"
        ".\r\n",
        sender, target, msg
    );
    client.send(message).unwrap("fail to send message");
    client.recv(buffer).unwrap("fail to receive message response");
    std::cout << "Received: " << buffer.data() << std::endl; // 打印接收到的消息响应
    buffer.fill(0);

    // quit
    client.send("QUIT\r\n").unwrap("fail to send QUIT");
    client.recv(buffer).unwrap("fail to receive QUIT response");
    std::cout << "Received: " << buffer.data() << std::endl; // 打印接收到的 QUIT 响应
}
