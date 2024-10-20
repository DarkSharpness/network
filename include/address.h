#pragma once
#include "strings.h"
#include "errors.h"
#include "utility.h"
#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <format>
#include <netdb.h>
#include <source_location>
#include <string_view>
#include <sys/socket.h>

namespace dark {

struct Address {
public:
    constexpr Address(std::uint32_t ip, std::uint16_t port) noexcept : _M_addr{} {
        _M_addr.sin_family = AF_INET;
        _M_addr.sin_port   = host_to_network(port);
        _M_addr.sin_addr   = {host_to_network(ip)};
    }

    constexpr Address(std::string_view ip, std::uint16_t port) noexcept : _M_addr{} {
        _M_addr.sin_family = AF_INET;
        _M_addr.sin_port   = host_to_network(port);
        _M_addr.sin_addr   = {string_to_ipv4_nocheck(ip)};
    }

    Address(cstring_view name, cstring_view service) noexcept : _M_addr{} {
        auto hints        = addrinfo{};
        hints.ai_family   = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        addrinfo *result;
        const auto ret = ::getaddrinfo(name.c_str(), service.c_str(), &hints, &result);
        if (ret != 0) {
            _M_invalidate(ret);
        } else {
            // Copy the result struct to sockaddr_in
            std::memcpy(&_M_addr, result->ai_addr, sizeof(_M_addr));
            ::freeaddrinfo(result);
        }
    }

    Address(cstring_view name, cstring_view service, std::uint16_t port) noexcept :
        Address(name, service) {
        if (this->port() == 0)
            this->port(port);
    }

    [[nodiscard]]
    constexpr auto unwrap(
        std::format_string<const char *> fmt = "{}",
        std::source_location loc             = std::source_location::current()
    ) const -> const Address & {
        const auto reason = _M_reason();
        if (reason != 0) // Failed to resolve the address
            assertion<bool, const char *>(false, fmt, gai_strerror(reason), loc);
        return *this;
    }

    [[nodiscard]]
    constexpr operator const sockaddr_in &() const noexcept {
        return _M_addr;
    }

    [[nodiscard]]
    constexpr bool is_valid() const noexcept {
        static_assert(sizeof(sockaddr_in::sin_zero) > 1);
        return _M_reason() == 0;
    }

    constexpr auto port() const noexcept -> std::uint16_t {
        return network_to_host(_M_addr.sin_port);
    }

    constexpr auto port(std::uint16_t port) noexcept -> Address & {
        _M_addr.sin_port = host_to_network(port);
        return *this;
    }

    constexpr auto ip() const noexcept -> std::uint32_t {
        return network_to_host(_M_addr.sin_addr.s_addr);
    }

    auto ip_str() const noexcept -> std::string {
        return ::inet_ntoa(_M_addr.sin_addr);
    }

    constexpr auto ip(std::uint32_t ip) noexcept -> Address & {
        _M_addr.sin_addr = {host_to_network(ip)};
        return *this;
    }

    constexpr auto ip(std::string_view ip) noexcept -> Address & {
        _M_addr.sin_addr = {string_to_ipv4_nocheck(ip)};
        return *this;
    }

private:
    static_assert(sizeof(sockaddr_in::sin_zero) >= sizeof(int));
    static_assert(offsetof(sockaddr_in, sin_zero) % alignof(int) == 0);

    auto _M_invalidate(int reason) noexcept -> void {
        *std::launder(reinterpret_cast<int *>(_M_addr.sin_zero)) = reason;
    }

    auto _M_reason() const noexcept -> int {
        return *std::launder(reinterpret_cast<const int *>(_M_addr.sin_zero));
    }

    alignas(alignof(int)) sockaddr_in _M_addr;
};

} // namespace dark
