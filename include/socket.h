#pragma once
#include "file.h"
#include "optional.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <new>
#include <string>
#include <string_view>
#include <sys/select.h>
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

template <bool _Empty = false>
struct SocketList;

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

using SocketRef = std::reference_wrapper<Socket>;

template <>
struct SocketList<false> : std::initializer_list<SocketRef> {
    using Super_t = std::initializer_list<SocketRef>;
    SocketList(Super_t list) noexcept : Super_t(list) {}
    auto list() const noexcept -> const Super_t & {
        return *this;
    }
};

template <>
struct SocketList<true> {
    SocketList(std::nullptr_t) noexcept {}
};

namespace __detail {

template <int _Tag>
struct FileSet<false, _Tag> {
public:
    FileSet(std::initializer_list<SocketRef> files) noexcept {
        FD_ZERO(&_M_set);
        for (const auto &file : files)
            FD_SET(file.get()._M_file.unsafe_get(), &_M_set);
    }
    FileSet(SocketList<false> files) noexcept : FileSet(files.list()) {}

    bool contains(const Socket &file) const noexcept {
        return FD_ISSET(file._M_file.unsafe_get(), &_M_set);
    }
    auto get() noexcept -> fd_set * {
        return &_M_set;
    }

private:
    fd_set _M_set;
};

template <int _Tag>
struct FileSet<true, _Tag> {
    FileSet(SocketList<true>) noexcept {}
    auto get() noexcept -> fd_set * {
        return nullptr;
    }
};

template <bool _EmptyR, bool _EmptyW, bool _EmptyE>
struct SelectResult {
    [[no_unique_address]]
    FileSet<_EmptyR, 0> reads;
    [[no_unique_address]]
    FileSet<_EmptyW, 1> writes;
    [[no_unique_address]]
    FileSet<_EmptyE, 2> excepts;
};

template <bool _EmptyR, bool _EmptyW, bool _EmptyE>
[[nodiscard]]
inline auto
select(SocketList<_EmptyR> reads, SocketList<_EmptyW> writes, SocketList<_EmptyE> excepts) noexcept
    -> optional<SelectResult<_EmptyR, _EmptyW, _EmptyE>> {
    auto result = SelectResult<_EmptyR, _EmptyW, _EmptyE>{
        .reads   = reads,
        .writes  = writes,
        .excepts = excepts,
    };
    const auto ret = ::select(
        FD_SETSIZE, result.reads.get(), result.writes.get(), result.excepts.get(), nullptr
    );
    if (ret < 0) {
        return erropt;
    } else {
        return result;
    }
}

using _List = std::initializer_list<SocketRef>;

} // namespace __detail

[[nodiscard]]
inline auto select(__detail::_List reads, __detail::_List writes, __detail::_List excepts) {
    return __detail::select<false, false, false>(reads, writes, excepts);
}

[[nodiscard]]
inline auto select(__detail::_List reads, __detail::_List writes, std::nullptr_t) {
    return __detail::select<false, false, true>(reads, writes, nullptr);
}

[[nodiscard]]
inline auto select(__detail::_List reads, std::nullptr_t, __detail::_List excepts) {
    return __detail::select<false, true, false>(reads, nullptr, excepts);
}

[[nodiscard]]
inline auto select(__detail::_List reads, std::nullptr_t, std::nullptr_t) {
    return __detail::select<false, true, true>(reads, nullptr, nullptr);
}

[[nodiscard]]
inline auto select(std::nullptr_t, __detail::_List writes, __detail::_List excepts) {
    return __detail::select<true, false, false>(nullptr, writes, excepts);
}

[[nodiscard]]
inline auto select(std::nullptr_t, __detail::_List writes, std::nullptr_t) {
    return __detail::select<true, false, true>(nullptr, writes, nullptr);
}

[[nodiscard]]
inline auto select(std::nullptr_t, std::nullptr_t, __detail::_List excepts) {
    return __detail::select<true, true, false>(nullptr, nullptr, excepts);
}

} // namespace dark
