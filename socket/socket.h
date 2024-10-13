#pragma once
#include "error.h"
#include <arpa/inet.h>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <new>
#include <optional>
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

    auto operator=(FileManager &&other) noexcept -> FileManager & {
        std::swap(_M_fd, other._M_fd);
        return *this;
    }

    auto valid() const noexcept -> bool {
        return _M_fd != _S_invalid;
    }

    explicit operator bool() const noexcept {
        return this->valid();
    }

    auto set(int fd) noexcept -> void {
        static_cast<void>(this->reset());
        _M_fd = fd;
    }

    [[nodiscard]]
    auto reset() noexcept -> bool {
        if (this->valid()) {
            return ::close(std::exchange(_M_fd, _S_invalid)) == 0;
        } else {
            return true;
        }
    }

    // Close the socket silently
    ~FileManager() noexcept {
        if (this->valid()) { // Avoid throwing exception in destructor
            static_cast<void>(::close(_M_fd));
        }
    }

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

struct Socket {
private:
    explicit Socket(FileManager file) noexcept : _M_file(std::move(file)) {}

public:
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

    explicit Socket(Domain dom, Type type, Protocol pro) :
        _M_file(::socket(static_cast<int>(dom), static_cast<int>(type), static_cast<int>(pro))) {
        assertion(_M_file, "Failed to create socket");
    }

    Socket(const Socket &)                     = delete;
    auto operator=(const Socket &) -> Socket & = delete;

    Socket(Socket &&other) noexcept                     = default;
    auto operator=(Socket &&other) noexcept -> Socket & = default;

    // Create a given sockaddr_in structure
    [[nodiscard]]
    static auto ip_port(const char *ip, std::uint16_t port) noexcept
        -> sockaddr_in {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(port);
        static_cast<void>(::inet_pton(AF_INET, ip, &addr.sin_addr));
        return addr;
    }

    using ReuseAddr = __detail::OptHelper<SO_REUSEADDR>;
    using Linger    = __detail::OptHelper<SO_LINGER, SOL_SOCKET, true>;
    using KeepAlive = __detail::OptHelper<SO_KEEPALIVE>;
    using NoDelay   = __detail::OptHelper<TCP_NODELAY, IPPROTO_TCP>;

    inline static constexpr auto reuse     = ReuseAddr{1};
    inline static constexpr auto linger    = Linger{1};
    inline static constexpr auto nolinger  = linger(0); // Disable linger
    inline static constexpr auto keepalive = KeepAlive{1};
    inline static constexpr auto nodelay   = NoDelay{1};

    template <int _Opt, int _Level = SOL_SOCKET, bool _Default>
    auto set_opt(const __detail::OptHelper<_Opt, _Level, _Default> &h) noexcept -> void {
        static_cast<void>(::setsockopt(
            _M_file.unsafe_get(), _Level, h._M_val.value_o, &h._M_val, sizeof(h._M_val)
        ));
    }

    [[nodiscard]]
    auto bind(const sockaddr_in &addr) noexcept -> bool {
        const auto *ptr = std::launder(reinterpret_cast<const sockaddr *>(&addr));
        return ::bind(_M_file.unsafe_get(), ptr, sizeof(addr)) == 0;
    }

    [[nodiscard]]
    auto connect(const sockaddr_in &addr) noexcept -> bool {
        const auto *ptr = std::launder(reinterpret_cast<const sockaddr *>(&addr));
        return ::connect(_M_file.unsafe_get(), ptr, sizeof(addr)) == 0;
    }

    [[nodiscard]]
    auto listen(int backlog) noexcept -> bool {
        return ::listen(_M_file.unsafe_get(), backlog) == 0;
    }

    [[nodiscard]]
    auto accept() noexcept -> std::optional<Socket> {
        sockaddr_in addr;
        socklen_t len = sizeof(addr);
        const auto fd = ::accept(_M_file.unsafe_get(), reinterpret_cast<sockaddr *>(&addr), &len);
        auto file     = FileManager{fd};
        return file ? std::optional{Socket{std::move(file)}} : std::nullopt;
    }

    template <Serializable _Tp>
    [[nodiscard]]
    auto receive(_Tp *buf, size_t len) noexcept -> std::optional<std::size_t> {
        const auto ret = ::recv(_M_file.unsafe_get(), buf, len, 0);
        return ret >= 0 ? std::optional{static_cast<std::size_t>(ret)} : std::nullopt;
    }

    template <Serializable _Tp>
    [[nodiscard]]
    auto send(const _Tp *buf, size_t len) noexcept -> std::optional<std::size_t> {
        const auto ret = ::send(_M_file.unsafe_get(), buf, len, 0);
        return ret >= 0 ? std::optional{static_cast<std::size_t>(ret)} : std::nullopt;
    }

    [[nodiscard]]
    auto is_connected() const noexcept -> bool {
        return _M_file.valid();
    }

    [[nodiscard]]
    explicit operator bool() const noexcept {
        return bool{_M_file};
    }

private:
    FileManager _M_file;
};

} // namespace dark