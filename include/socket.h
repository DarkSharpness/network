#pragma once
#include "file.h"
#include "optional.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <new>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace dark {

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

template <bool _Empty, int _Tag>
struct FileSet;

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

struct Socket {
private:
    explicit Socket(FileManager file) noexcept : _M_file(std::move(file)) {}
    friend struct __detail::FileSet<false, 0>;
    friend struct __detail::FileSet<false, 1>;
    friend struct __detail::FileSet<false, 2>;

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
