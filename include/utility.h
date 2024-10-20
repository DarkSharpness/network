#pragma once
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <netinet/in.h>
#include <string_view>

namespace dark {

namespace __detail {

inline constexpr bool is_big_endian    = (std::endian::native == std::endian::big);
inline constexpr bool is_little_endian = (std::endian::native == std::endian::little);

} // namespace __detail

template <std::integral _Int>
inline constexpr auto endian_transform(_Int value) noexcept -> _Int {
    return std::byteswap(value);
}

template <std::integral _Int>
inline constexpr auto host_to_network(_Int value) noexcept -> _Int {
    if constexpr (__detail::is_big_endian) {
        return value;
    } else {
        return endian_transform(value);
    }
}

template <std::unsigned_integral _Int>
inline constexpr auto network_to_host(_Int value) noexcept -> _Int {
    if constexpr (__detail::is_big_endian) {
        return value;
    } else {
        return endian_transform(value);
    }
}

template <std::uint32_t base = 10>
inline constexpr auto string_to_ipv4_nocheck(std::string_view str) noexcept -> in_addr_t {
    static_assert(std::numeric_limits<std::uint8_t>::digits == 8, "std::uint8_t must be 8 bits");
    in_addr_t addr     = 0;
    std::uint8_t octet = 0;
    std::size_t offset = 0;
    for (const std::uint8_t c : str) {
        if (c == '.') {
            addr |= host_to_network(octet) << offset;
            offset += 8;
            octet = 0;
        } else {
            octet = octet * base + (c - '0');
        }
    }
    addr |= host_to_network(octet) << offset;
    return addr;
}

template <std::integral _Int, std::size_t base = 10>
inline constexpr auto str_to_int_nocheck(std::string_view str) noexcept -> _Int {
    _Int value{};
    for (const std::uint8_t c : str)
        value = value * base + (c - '0');
    return value;
}

} // namespace dark
