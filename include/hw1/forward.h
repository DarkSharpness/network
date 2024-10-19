#pragma once
#include <cstdint>
#include <string_view>

auto send_mail(std::string_view msg, std::string_view sender, std::string_view target) -> void;
auto run_proxy(std::string_view ip, std::uint16_t port) -> void;
