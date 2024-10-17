#pragma once
#include <concepts>
#include <format>
#include <iostream>
#include <source_location>
#include <string_view>

namespace dark {

namespace __detail {

// A consteval c-style string
struct const_string {
public:
    consteval const_string(const char *str) : _M_str(str) {}
    auto c_str() const -> const char * {
        return _M_str.data();
    }
    auto length() const -> std::size_t {
        return _M_str.size();
    }

private:
    const std::string_view _M_str;
};

struct no_assertion_msg_tag {};

template <typename... _Args>
concept no_assertion_msg = (std::same_as<_Args, no_assertion_msg_tag> || ...);

} // namespace __detail

template <typename... _Args>
[[noreturn]] inline void panic(
    std::format_string<_Args...> fmt, _Args &&...args,
    std::source_location loc      = std::source_location::current(),
    __detail::const_string detail = "program panicked"
) {
    std::cerr << std::format(
        "{} at {}:{}: {}\n", detail.c_str(), loc.file_name(), loc.line(),
        std::format(fmt, std::forward<_Args>(args)...)
    );
    throw std::runtime_error(detail.c_str());
}

template <typename _Cond = bool, typename... _Args>
    requires std::constructible_from<bool, _Cond>
struct assertion {
private:
    using _Source = std::source_location;

public:
    explicit assertion(
        _Cond &&cond, std::format_string<_Args...> fmt, _Args &&...args,
        _Source loc = _Source::current() // Need c++20
    )
        requires(!__detail::no_assertion_msg<_Args...>)
    {
        if (cond) [[likely]]
            return;
        // Explicitly specify the type of the arguments to avoid ambiguity
        panic<_Args...>(fmt, std::forward<_Args>(args)..., loc, "assertion failed");
    }

    explicit assertion(_Cond &&cond, _Source loc = _Source::current())
        requires(__detail::no_assertion_msg<_Args...>)
    {
        if (cond) [[likely]]
            return;
        // Explicitly specify the type of the arguments to avoid ambiguity
        panic<const char *>("{}", loc.function_name(), loc, "assertion failed");
    }
};

template <typename _Cond, typename... _Args>
explicit assertion(_Cond &&, std::format_string<_Args...>, _Args &&...)
    -> assertion<_Cond, _Args...>;

template <typename _Cond>
explicit assertion(_Cond &&) -> assertion<_Cond, __detail::no_assertion_msg_tag>;

} // namespace dark
