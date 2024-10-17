#pragma once

#include "errors.h"
#include <algorithm>
#include <format>
#include <memory>
#include <source_location>
#include <type_traits>

namespace dark {

namespace __detail {

struct optional_base {
    struct nullopt_t {};
    inline static constexpr char _S_default[] = "optional not initialized";
};

} // namespace __detail

inline constexpr __detail::optional_base::nullopt_t nullopt = {};

template <typename _Tp>
struct optional : private __detail::optional_base {
private:
    using __detail::optional_base::_S_default;
    using _Source = std::source_location;
    using _Fmt    = std::format_string<const char *&>;

    inline static constexpr bool _S_noexcept_move    = std::is_nothrow_move_constructible_v<_Tp>;
    inline static constexpr bool _S_trivial_destruct = std::is_trivially_destructible_v<_Tp>;

    auto _M_destory() noexcept -> void {
        if (this->has_value())
            std::destroy_at(&_M_value);
        _M_error = _S_default;
    }

    // precondition: _M_error is not nullptr
    template <typename... _Args>
    auto _M_unsafe_new(_Args &&...args) noexcept -> void {
        _M_error = nullptr;
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

    optional(nullopt_t = {}) noexcept : _M_error(_S_default) {}
    explicit optional(nullopt_t, const char *msg) noexcept : _M_error(msg) {}

    template <typename _Up>
    // clang-format off
        requires(!std::is_same_v<std::remove_cv_t<_Up>, optional> &&
                 !std::is_same_v<std::remove_cv_t<_Up>, nullopt_t>)
    // clang-format on
    optional(_Up &&value) noexcept(std::is_nothrow_constructible_v<_Tp, _Up>) {
        _M_unsafe_new(std::forward<_Up>(value));
    }

    optional(const optional &other)            = delete;
    optional &operator=(const optional &other) = delete;

    optional(optional &&other) noexcept(_S_noexcept_move) {
        if (other.has_value()) {
            _M_unsafe_new(std::move(other._M_value));
        } else {
            _M_error = _S_default;
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

    auto unwrap(_Fmt fmt = "{}", _Source src = _Source::current()) -> _Tp {
        assertion<bool, const char *&>(this->has_value(), fmt, _M_error, src);
        _Tp return_value = std::move(_M_value);
        this->_M_destory();
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

    auto has_value() const noexcept -> bool {
        return _M_error == nullptr;
    }

    explicit operator bool() const noexcept {
        return this->has_value();
    }

private:
    const char *_M_error;
    union {
        char _M_dummy; // Avoid empty union (?)
        _Tp _M_value;
    };
};

template <typename _Tp>
optional(_Tp &&) -> optional<std::remove_cvref_t<_Tp>>;

} // namespace dark
