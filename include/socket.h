#pragma once
#include "cstring.h"
#include "errors.h"
#include "optional.h"
#include "utility.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <new>
#include <source_location>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <type_traits>
#include <unistd.h>
#include <utility>

namespace dark {

template <typename _Tp>
concept Serializable = std::is_trivially_copyable_v<_Tp> && std::is_trivially_destructible_v<_Tp>;

struct FileManager {
public:
    explicit FileManager() noexcept : _M_fd(_S_invalid) {}
    explicit FileManager(int fd) noexcept : _M_fd(fd) {}

    FileManager(const FileManager &)                     = delete;
    auto operator=(const FileManager &) -> FileManager & = delete;

    FileManager(FileManager &&other) noexcept : _M_fd(std::exchange(other._M_fd, _S_invalid)) {}

    // Close the socket silently
    ~FileManager() noexcept {
        if (this->valid() && ::close(_M_fd) != 0)
            errno = 0; // Ignore the error, avoid throwing exception
    }

    auto operator=(FileManager &&other) noexcept -> FileManager & {
        _M_fd = std::exchange(other._M_fd, _S_invalid);
        return *this;
    }

    [[nodiscard]]
    auto valid() const noexcept -> bool {
        return _M_fd != _S_invalid;
    }

    [[nodiscard]]
    explicit operator bool() const noexcept {
        return this->valid();
    }

    auto set(int fd) noexcept -> void {
        static_cast<void>(this->reset());
        _M_fd = fd;
    }

    [[nodiscard]]
    auto reset() noexcept -> optional<> {
        if (this->valid()) {
            return ::close(std::exchange(_M_fd, _S_invalid)) == 0;
        } else {
            return true;
        }
    }

    [[nodiscard]]
    auto unsafe_get() const noexcept -> int {
        return _M_fd;
    }

private:
    static constexpr int _S_invalid = -1;
    int _M_fd;
};

struct Socket;

namespace __detail {

// Helper class to set socket options
template <int _Opt, int _Level = SOL_SOCKET, bool _Construct = false>
struct OptHelper {
public:
    friend struct ::dark::Socket;
    friend struct OptHelper<_Opt, _Level, true>;

    constexpr auto operator()(int val) const noexcept -> OptHelper<_Opt, _Level, false>
        requires _Construct // Construct a new object if the current object is not initialized
    {
        return OptHelper<_Opt, _Level, false>{val};
    }

private:
    constexpr OptHelper(int val) noexcept : _M_val(val) {}
    const int _M_val;
};

} // namespace __detail

enum class Domain {
    INET4 = AF_INET,
    INET6 = AF_INET6,
};
enum class Type {
    STREAM = SOCK_STREAM,
};
enum class Protocol {
    DEFAULT = IPPROTO_IP,
    TCP     = IPPROTO_TCP,
    UDP     = IPPROTO_UDP,
};

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
        _M_addr.sin_addr   = {string_to_ipv4_noexcept(ip)};
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
    constexpr operator const sockaddr_in &() const {
        return this->unwrap()._M_addr;
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
        _M_addr.sin_addr = {string_to_ipv4_noexcept(ip)};
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

struct Socket {
private:
    explicit Socket(FileManager file) noexcept : _M_file(std::move(file)) {}

public:

    explicit Socket(Domain dom, Type type, Protocol pro) :
        _M_file(::socket(static_cast<int>(dom), static_cast<int>(type), static_cast<int>(pro))) {}

    Socket(const Socket &)                     = delete;
    auto operator=(const Socket &) -> Socket & = delete;

    Socket(Socket &&other) noexcept                     = default;
    auto operator=(Socket &&other) noexcept -> Socket & = default;

    using ReuseAddr = __detail::OptHelper<SO_REUSEADDR>;
    using Linger    = __detail::OptHelper<SO_LINGER, SOL_SOCKET, true>;
    using KeepAlive = __detail::OptHelper<SO_KEEPALIVE>;
    using NoDelay   = __detail::OptHelper<TCP_NODELAY, IPPROTO_TCP>;

    inline static constexpr auto opt_reuse     = ReuseAddr{1};
    inline static constexpr auto opt_linger    = Linger{1};
    inline static constexpr auto opt_nolinger  = opt_linger(0); // Disable linger
    inline static constexpr auto opt_keepalive = KeepAlive{1};
    inline static constexpr auto opt_nodelay   = NoDelay{1};

    template <int _Opt, int _Level, bool _>
    [[nodiscard]]
    auto set_opt(const __detail::OptHelper<_Opt, _Level, _> &h) noexcept -> optional<> {
        return ::setsockopt(_M_file.unsafe_get(), _Level, _Opt, &h._M_val, sizeof(h._M_val)) == 0;
    }

    [[nodiscard]]
    auto bind(const sockaddr_in &addr) noexcept -> optional<> {
        const auto *ptr = std::launder(reinterpret_cast<const sockaddr *>(&addr));
        return ::bind(_M_file.unsafe_get(), ptr, sizeof(addr)) == 0;
    }

    [[nodiscard]]
    auto connect(const sockaddr_in &addr) noexcept -> optional<> {
        const auto *ptr = std::launder(reinterpret_cast<const sockaddr *>(&addr));
        return ::connect(_M_file.unsafe_get(), ptr, sizeof(addr)) == 0;
    }

    [[nodiscard]]
    auto listen(int backlog) noexcept -> optional<> {
        return ::listen(_M_file.unsafe_get(), backlog) == 0;
    }

    [[nodiscard]]
    auto accept() noexcept -> optional<std::pair<Socket, sockaddr_in>> {
        sockaddr_in addr;
        socklen_t len = sizeof(addr);
        const auto fd = ::accept(_M_file.unsafe_get(), reinterpret_cast<sockaddr *>(&addr), &len);
        if (auto file = FileManager{fd}) {
            return std::pair{Socket{std::move(file)}, addr};
        } else {
            return erropt;
        }
    }

    [[nodiscard]]
    auto recv(std::string &buffer) noexcept -> optional<std::size_t> {
        // Resize the buffer to the maximum size, but without ovewriting the data
        buffer.resize_and_overwrite(buffer.capacity(), [](char *, std::size_t len) { return len; });
        const auto ret = ::recv(_M_file.unsafe_get(), buffer.data(), buffer.size(), 0);
        if (ret < 0) {
            buffer.clear();
            return erropt;
        } else {
            buffer.resize(static_cast<std::size_t>(ret));
            return static_cast<std::size_t>(ret);
        }
    }

    [[nodiscard]]
    auto send(std::string_view str) noexcept -> optional<std::size_t> {
        const auto ret = ::send(_M_file.unsafe_get(), str.data(), str.size(), 0);
        if (ret < 0) {
            return erropt;
        } else {
            return static_cast<std::size_t>(ret);
        }
    }

    [[nodiscard]]
    auto is_valid() const noexcept -> bool {
        return _M_file.valid();
    }

    [[nodiscard]]
    explicit operator bool() const noexcept {
        return this->is_valid();
    }

    [[nodiscard, deprecated("only use _debug functions in debug mode")]]
    auto _debug_unsafe_get() const noexcept -> int {
        return _M_file.unsafe_get();
    }

private:
    FileManager _M_file;
};

} // namespace dark
