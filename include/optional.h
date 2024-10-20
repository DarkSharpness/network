#pragma once

#include "errors.h"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <format>
#include <memory>
#include <source_location>
#include <type_traits>
#include <utility>

namespace dark {

namespace __detail {

struct optional_base {
public:
    struct nullopt_t {};
    struct erropt_t {};

protected:
    optional_base() noexcept                        = default;
    optional_base(const optional_base &)            = delete;
    optional_base &operator=(const optional_base &) = delete;
    using _Errno_t                                  = int;
    using _Fmt_t                                    = std::format_string<const char *&>;
    using _Src_t                                    = std::source_location;

    inline static constexpr auto _S_default = _Errno_t{-1};
    inline static constexpr auto _S_noerror = _Errno_t{0};
};

} // namespace __detail

inline constexpr __detail::optional_base::nullopt_t nullopt = {};
inline constexpr __detail::optional_base::erropt_t erropt   = {};

template <typename _Tp = void>
struct optional : private __detail::optional_base {
private:
    inline static constexpr bool _S_noexcept_move    = std::is_nothrow_move_constructible_v<_Tp>;
    inline static constexpr bool _S_trivial_destruct = std::is_trivially_destructible_v<_Tp>;

    auto _M_destory() noexcept -> void {
        if (this->has_value())
            std::destroy_at(&_M_value);
        _M_errno = _S_default;
    }

    // precondition: _M_error is not nullptr
    template <typename... _Args>
    auto _M_unsafe_new(_Args &&...args) noexcept -> void {
        _M_errno = {};
        std::construct_at(&_M_value, std::forward<_Args>(args)...);
    }

public:

    ~optional() noexcept
        requires(_S_trivial_destruct)
    = default;
    ~optional() noexcept
        requires(!_S_trivial_destruct)
    {
        _M_destory();
    }

    optional(nullopt_t = {}) noexcept : _M_errno(_S_default) {}
    optional(erropt_t) noexcept : _M_errno(errno) {}

    template <typename _Up>
    // clang-format off
        requires(!std::is_same_v<std::remove_cv_t<_Up>, optional> &&
                 !std::is_same_v<std::remove_cv_t<_Up>, nullopt_t>)
    // clang-format on
    optional(_Up &&value) noexcept(std::is_nothrow_constructible_v<_Tp, _Up>) {
        _M_unsafe_new(std::forward<_Up>(value));
    }

    optional(optional &&other) noexcept(_S_noexcept_move) {
        if (other.has_value()) {
            _M_unsafe_new(std::move(other._M_value));
        } else {
            _M_errno = _S_default;
        }
    }

    optional &operator=(optional &&other) noexcept(_S_noexcept_move) {
        if (this != &other) {
            std::destroy_at(this);
            std::construct_at(this, std::move(other));
        }
        return *this;
    }

    template <typename... _Args>
    auto emplace(_Args &&...args) noexcept(std::is_nothrow_constructible_v<_Tp, _Args...>) -> void {
        _M_destory();
        _M_unsafe_new(std::forward<_Args>(args)...);
    }

    template <typename _Up>
    auto assign(_Up &&value)
        // clang-format off
        noexcept(std::is_nothrow_assignable_v<_Tp, _Up> && std::is_nothrow_constructible_v<_Tp, _Up>)
        // clang-format on
        -> void {
        if (this->has_value())
            _M_value = std::forward<_Up>(value);
        else
            _M_unsafe_new(std::forward<_Up>(value));
    }

    auto unwrap(_Fmt_t fmt = "{}", _Src_t src = _Src_t::current()) -> _Tp {
        if (!this->has_value()) { // slow path
            using _Assertion_t    = assertion<bool, const char *&>;
            const char *error_str = "using an empty optional";
            if (_M_errno != _S_default)
                error_str = ::strerror(_M_errno);
            _Assertion_t(false, fmt, error_str, src);
        }

        _Tp return_value = std::move(_M_value);
        _M_destory();
        return return_value;
    }

    template <typename _Up>
    auto value_or(_Up &&def) const & noexcept -> _Tp {
        return this->has_value() ? _M_value : std::forward<_Up>(def);
    }

    template <typename _Up>
    auto value_or(_Up &&def) && noexcept -> _Tp {
        return this->has_value() ? std::move(_M_value) : std::forward<_Up>(def);
    }

    auto discard() noexcept -> void {
        _M_destory();
    }

    auto has_value() const noexcept -> bool {
        return _M_errno == _S_noerror;
    }

    explicit operator bool() const noexcept {
        return this->has_value();
    }

private:
    union {
        char _M_dummy; // Avoid empty union (?)
        _Tp _M_value;
    };
    _Errno_t _M_errno;
};

template <>
struct optional<void> : private __detail::optional_base {
public:
    ~optional() noexcept = default;
    optional(nullopt_t = {}) noexcept : _M_errno(_S_default) {}
    optional(erropt_t) noexcept : _M_errno(errno) {}
    optional(bool success) noexcept : _M_errno(success ? _S_noerror : errno) {}

    optional(const optional &other)            = delete;
    optional &operator=(const optional &other) = delete;

    optional(optional &&other) noexcept : _M_errno(std::exchange(other._M_errno, _S_default)) {}

    optional &operator=(optional &&other) noexcept {
        _M_errno = std::exchange(other._M_errno, _S_default);
        return *this;
    }

    auto unwrap(_Fmt_t fmt = "{}", _Src_t src = _Src_t::current()) -> void {
        if (!this->has_value()) { // slow path
            using _Assertion_t    = assertion<bool, const char *&>;
            const char *error_str = "using an empty optional";
            if (_M_errno != _S_default)
                error_str = ::strerror(_M_errno);
            _Assertion_t(false, fmt, error_str, src);
        }
        _M_errno = _S_default;
    }

    auto discard() noexcept -> void {
        _M_errno = _S_default;
    }

    auto has_value() const noexcept -> bool {
        return _M_errno == _S_noerror;
    }

    explicit operator bool() const noexcept {
        return this->has_value();
    }

private:
    _Errno_t _M_errno;
};

template <typename _Tp>
optional(_Tp &&) -> optional<std::remove_cvref_t<_Tp>>;

} // namespace dark
