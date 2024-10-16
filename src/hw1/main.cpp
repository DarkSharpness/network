#include "hw1/forward.h"
#include <format>
#include <iostream>
#include <string>


auto main() -> int {
    std::cout <<
        R"(choose an demo to run:
    1. send a mail to students in SJTU
    2. proxies
your choice:
)";
    std::string sender;
    std::string receiver;
    std::string message;
    int x;
    std::cin >> x;
    if (x == 1) {
        while (std::cin) {
            std::cout << "Please enter your name (sender): ";
            std::cin >> sender;
            std::cout << "Please enter the receiver's name: ";
            std::cin >> receiver;
            std::cout << "Please enter the message: ";
            std::cin.ignore();
            std::getline(std::cin, message);
            std::cout << "Please verify the settting:\n";
            std::cout << std::format(
                "- sender:   {}@sjtu.edu.cn\n"
                "- receiver: {}@sjtu.edu.cn\n"
                "- message:  {}\n",
                sender, receiver, message
            );
            std::cout << "Is it correct? (y/n): ";
            std::string ans;
            std::cin >> ans;
            if (ans == "y") {
                sender   = std::format("{}@sjtu.edu.cn", sender);
                receiver = std::format("{}@sjtu.edu.cn", receiver);
                send_mail(message, sender, receiver);
                break;
            }
            std::cout << "Please re-enter the information:\n";
        }
    } else if (x == 2) {
        run_proxy("127.0.0.1", 4321);
    }
    std::cout << "bye\n";
    return 0;
}
