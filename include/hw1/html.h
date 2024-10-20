#pragma once
#include "address.h"
#include "socket.h"
#include "utility.h"
#include <string_view>

inline auto parse_http(std::string_view str, std::string_view key, std::string_view end = "\r\n")
    -> std::string_view {
    auto pos = str.find(key);
    if (pos == std::string_view::npos)
        return {};
    auto value = str.substr(pos + key.size());
    return value.substr(0, value.find(end));
}

template <typename _Op>
inline auto receive_http(dark::Socket &conn, std::string &buffer, _Op &&op) -> void {
    // Keep reading until the connection is closed
    auto header_length  = std::size_t{};
    auto content_length = std::size_t{};
    auto length         = std::size_t{};

    while (true) {
        conn.recv(buffer).unwrap("recv failed: {}");
        op(std::as_const(buffer));
        if (buffer.size() == 0)
            break;

        length += buffer.size();
        if (header_length == 0) {
            auto header = std::string_view{buffer};
            auto pos    = header.find("\r\n\r\n");
            if (pos == std::string_view::npos)
                continue; // Header is not complete yet
            // Header is complete
            header_length  = pos + 4;
            auto content   = parse_http(header, "Content-Length: ");
            content_length = dark::str_to_int_nocheck<std::size_t>(content);
        }

        // if here, header is complete
        if (length >= header_length + content_length)
            break;
    }
}

inline auto receive_http(dark::Socket &conn, std::string &buffer) -> std::string {
    auto message = std::string{};
    receive_http(conn, buffer, [&](std::string_view data) { message += data; });
    return message;
}

inline auto parse_host(std::string_view str) -> std::optional<std::pair<dark::Address, bool>> {
    auto pos = str.find("://");
    if (pos == std::string_view::npos) {
        pos = str.find(':');
        if (pos == std::string_view::npos)
            return {};
        auto host = str.substr(0, pos);
        auto port = dark::str_to_int_nocheck<std::uint16_t>(str.substr(pos + 1));
        return std::make_pair(dark::Address{std::string{host}, "", port}, false);
    } else {
        // We support only http now.
        if (!str.starts_with("http://"))
            return {};
        str = str.substr(7);
        pos = str.find('/');
        if (pos == std::string_view::npos)
            return std::nullopt;
        auto host = str.substr(0, pos);
        return std::make_pair(dark::Address{std::string{host}, "http"}, true);
    }
}
