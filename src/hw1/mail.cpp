#include "hw1/forward.h"
#include "socket.h"
#include <format>
#include <iostream>
#include <netdb.h>
#include <string>
#include <string_view>
#include <sys/socket.h>

using dark::Address;
using dark::Socket;

auto send_mail(std::string_view msg, std::string_view sender, std::string_view target) -> void {
    // Initialize a socket and connect to the mail server
    const auto mail_server = Address{"mail.sjtu.edu.cn", "http", 25};
    auto client            = Socket{dark::Domain::INET4, dark::Type::STREAM, dark::Protocol::TCP};
    client.connect(mail_server).unwrap("fail to connect to mail server");

    // accept a welcome message
    auto buffer = std::string(1024, '\0');
    client.recv(buffer).unwrap("fail to receive welcome message");
    std::cout << "Received: " << buffer << std::endl;

    // send HELO
    client.send("HELO DarkSharpness\r\n").unwrap("fail to send HELO");
    client.recv(buffer).unwrap("fail to receive HELO response");

    if (!buffer.starts_with("250")) {
        // Retry with HELO, a problem of SJTU mail server
        client.send("HELO DarkSharpness\r\n").unwrap("fail to send HELO");
        client.recv(buffer).unwrap("fail to receive HELO response");
    }

    std::cout << "Received: " << buffer << std::endl;
    std::cout << "Now sending mail..." << std::endl;

    // send MAIL FROM
    client.send(std::format("MAIL FROM: <{}>\r\n", sender)).unwrap("fail to send MAIL FROM");
    client.recv(buffer).unwrap("fail to receive MAIL FROM response");
    std::cout << "Received: " << buffer << std::endl;

    // send RCPT TO
    client.send(std::format("RCPT TO: <{}>\r\n", target)).unwrap("fail to send RCPT TO");
    client.recv(buffer).unwrap("fail to receive RCPT TO response");
    std::cout << "Received: " << buffer << std::endl;

    // send DATA
    client.send("DATA\r\n").unwrap("fail to send DATA");
    client.recv(buffer).unwrap("fail to receive DATA response");
    std::cout << "Received: " << buffer << std::endl;

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
    std::cout << "Received: " << buffer << std::endl; // 打印接收到的消息响应

    // quit
    client.send("QUIT\r\n").unwrap("fail to send QUIT");
    client.recv(buffer).unwrap("fail to receive QUIT response");
    std::cout << "Received: " << buffer << std::endl; // 打印接收到的 QUIT 响应
}
