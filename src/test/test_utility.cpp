#include "unit_test.h"
#include "utility.h"
#include <arpa/inet.h>
#include <format>
#include <iostream>

static auto test() -> void {
    auto data = ::inet_addr("0.0.0.1");
    std::cout << std::format("data: {}\n", data);
    data = dark::string_to_ipv4_noexcept("0.0.0.1");
    std::cout << std::format("data: {}\n", data);
}

static auto testcase = Testcase(test);
