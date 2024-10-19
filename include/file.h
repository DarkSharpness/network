#pragma once
#include "optional.h"
#include <cerrno>
#include <type_traits>
#include <unistd.h>
#include <utility>

namespace dark {

template <typename _Tp>
concept Serializable =
    std::is_trivially_copy_constructible_v<_Tp> && std::is_trivially_destructible_v<_Tp>;

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

} // namespace dark
