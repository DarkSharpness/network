#pragma once
#include "optional.h"
#include <cerrno>
#include <unistd.h>
#include <utility>

namespace dark {

struct FileManager {
private:
    using _Fd_t                       = int;
    static constexpr _Fd_t _S_invalid = -1;

public:
    explicit FileManager(_Fd_t fd = _S_invalid) noexcept : _M_fd(fd) {}

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
        return _M_fd >= 0;
    }

    [[nodiscard]]
    explicit operator bool() const noexcept {
        return this->valid();
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
    auto unsafe_get() const noexcept -> _Fd_t {
        return _M_fd;
    }

private:
    _Fd_t _M_fd;
};

} // namespace dark
