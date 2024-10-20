#pragma once
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <string>
#include <string_view>
#include <strings.h>
#include <type_traits>
#include <utility>

namespace dark {

struct cstring_view;
struct consteval_string;
template <std::size_t _Nm>
struct stack_string;

template <typename _Tp>
struct cstring_traits : std::false_type {
    static_assert(
        std::same_as<_Tp, std::remove_cvref_t<_Tp>>, "cstring_traits: _Tp must be a non-cvref type"
    );
};

template <>
struct cstring_traits<std::string> : std::true_type {};
template <>
struct cstring_traits<consteval_string> : std::true_type {};
template <std::size_t _Nm>
struct cstring_traits<stack_string<_Nm>> : std::true_type {};

template <typename _Tp>
concept null_terminated_string = cstring_traits<_Tp>::value;

struct cstring_view {
private:
    using _Sv_t = std::string_view;
    struct _Unsafe_t {};
    constexpr cstring_view(const char *data, std::size_t length, _Unsafe_t) noexcept :
        _M_sv(data, length) {}

public:
    using value_type             = const char;
    using pointer                = value_type *;
    using const_pointer          = const char *;
    using reference              = value_type &;
    using const_reference        = const value_type &;
    using const_iterator         = const value_type *;
    using iterator               = const_iterator;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reverse_iterator       = const_reverse_iterator;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;

    static constexpr std::size_t npos = std::string_view::npos;

public:
    constexpr cstring_view() noexcept : _M_sv() {}
    constexpr cstring_view(const char *data) noexcept : _M_sv(data) {}

    template <null_terminated_string _Str>
    constexpr cstring_view(const _Str &str) noexcept : _M_sv(std::data(str), std::size(str)) {}

    constexpr cstring_view(const char *data, std::size_t length) noexcept {
        if (data[length] != '\0')
            *this = cstring_view{data};
        else
            _M_sv = _Sv_t{data, length};
    }

    constexpr operator std::string_view() const noexcept {
        return _M_sv;
    }
    explicit constexpr operator std::string() const noexcept {
        return std::string{_M_sv};
    }

    constexpr auto c_str() const noexcept -> const_pointer {
        return _M_sv.data();
    }
    constexpr auto data() const noexcept -> const_pointer {
        return _M_sv.data();
    }
    constexpr auto length() const noexcept -> size_type {
        return _M_sv.length();
    }
    constexpr auto size() const noexcept -> size_type {
        return _M_sv.size();
    }
    constexpr auto empty() const noexcept -> bool {
        return _M_sv.empty();
    }
    constexpr auto front() const noexcept -> const_reference {
        return _M_sv.front();
    }
    constexpr auto back() const noexcept -> const_reference {
        return _M_sv.back();
    }
    constexpr auto operator[](std::size_t pos) const noexcept -> const_reference {
        return _M_sv[pos];
    }
    constexpr auto at(std::size_t pos) const -> const_reference {
        return _M_sv.at(pos);
    }
    constexpr auto begin() const noexcept -> const_iterator {
        return _M_sv.begin();
    }
    constexpr auto end() const noexcept -> const_iterator {
        return _M_sv.end();
    }

    // Remark: cstring must be null-terminated
    constexpr auto substr(std::size_t pos) const noexcept -> cstring_view {
        return cstring_view{_M_sv.data() + pos, _M_sv.size() - pos, {}};
    }

    constexpr auto split_at(std::size_t pos) const noexcept -> std::pair<_Sv_t, cstring_view> {
        return std::make_pair(_M_sv.substr(0, pos), cstring_view{_M_sv.data() + pos});
    }

    constexpr auto pop_front(std::size_t n = 1) noexcept -> _Sv_t {
        auto ret = _M_sv.substr(0, n);
        _M_sv.remove_prefix(n);
        return ret;
    }

    constexpr auto find(cstring_view sv, std::size_t pos = 0) const noexcept -> size_type {
        return _Sv_t{*this}.find(sv, pos);
    }

    constexpr auto rfind(cstring_view sv, std::size_t pos = std::string::npos) const noexcept
        -> size_type {
        return _Sv_t{*this}.rfind(sv, pos);
    }

    constexpr auto find_first_of(cstring_view sv, std::size_t pos = 0) const noexcept -> size_type {
        return _Sv_t{*this}.find_first_of(sv, pos);
    }

    constexpr auto find_last_of(cstring_view sv, std::size_t pos = std::string::npos) const noexcept
        -> size_type {
        return _Sv_t{*this}.find_last_of(sv, pos);
    }

    constexpr auto find_first_not_of(cstring_view sv, std::size_t pos = 0) const noexcept
        -> size_type {
        return _Sv_t{*this}.find_first_not_of(sv, pos);
    }

    constexpr auto
    find_last_not_of(cstring_view sv, std::size_t pos = std::string::npos) const noexcept
        -> size_type {
        return _Sv_t{*this}.find_last_not_of(sv, pos);
    }

    template <typename... _Args>
        requires requires(_Sv_t sv, _Args &&...args) { sv.compare(std::forward<_Args>(args)...); }
    constexpr auto compare(_Args &&...args) const noexcept -> int {
        return _Sv_t{*this}.compare(std::forward<_Args>(args)...);
    }

    template <typename... _Args>
        requires requires(_Sv_t sv, _Args &&...args) {
            sv.starts_with(std::forward<_Args>(args)...);
        }
    constexpr auto starts_with(_Args &&...args) const noexcept -> bool {
        return _Sv_t{*this}.starts_with(std::forward<_Args>(args)...);
    }

    template <typename... _Args>
        requires requires(_Sv_t sv, _Args &&...args) { sv.ends_with(std::forward<_Args>(args)...); }
    constexpr auto ends_with(_Args &&...args) const noexcept -> bool {
        return _Sv_t{*this}.ends_with(std::forward<_Args>(args)...);
    }

    constexpr auto operator<=>(cstring_view sv) const noexcept {
        return _Sv_t{*this} <=> _Sv_t{sv};
    }

    constexpr auto operator==(cstring_view sv) const noexcept -> bool {
        return _Sv_t{*this} == _Sv_t{sv};
    }

private:
    std::string_view _M_sv;
};

// A compile-time string
struct consteval_string {
public:
    using value_type             = const char;
    using pointer                = value_type *;
    using const_pointer          = const char *;
    using reference              = value_type &;
    using const_reference        = const value_type &;
    using const_iterator         = const value_type *;
    using iterator               = const_iterator;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reverse_iterator       = const_reverse_iterator;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;

public:
    consteval consteval_string(const char *data) : _M_sv(data) {}
    consteval consteval_string(const cstring_view &csv) : _M_sv(csv) {}

    constexpr auto c_str() const noexcept -> const_pointer {
        return _M_sv.data();
    }

    constexpr auto data() const noexcept -> const_pointer {
        return _M_sv.data();
    }

    constexpr auto length() const noexcept -> size_type {
        return _M_sv.length();
    }

    constexpr auto size() const noexcept -> size_type {
        return _M_sv.size();
    }

    constexpr auto empty() const noexcept -> bool {
        return _M_sv.empty();
    }

    constexpr auto front() const noexcept -> const_reference {
        return _M_sv.front();
    }

    constexpr auto back() const noexcept -> const_reference {
        return _M_sv.back();
    }

    constexpr auto operator[](std::size_t pos) const noexcept -> const_reference {
        return _M_sv[pos];
    }

    constexpr auto at(std::size_t pos) const -> const_reference {
        return _M_sv.at(pos);
    }

    constexpr auto begin() const noexcept -> const_iterator {
        return _M_sv.begin();
    }

    constexpr auto end() const noexcept -> const_iterator {
        return _M_sv.end();
    }

    constexpr auto view() const noexcept -> cstring_view {
        return cstring_view{*this};
    }

    constexpr operator std::string_view() const noexcept {
        return std::string_view{_M_sv};
    }

    explicit constexpr operator std::string() const noexcept {
        return std::string{_M_sv};
    }

    constexpr auto operator<=>(const consteval_string &other) const noexcept {
        return _M_sv <=> other._M_sv;
    }

    constexpr auto operator==(const consteval_string &other) const noexcept -> bool {
        return _M_sv == other._M_sv;
    }

private:
    const std::string_view _M_sv;
};

// A string that must be on the stack
template <std::size_t _Nm>
struct stack_string {
public:
    using value_type             = char;
    using pointer                = value_type *;
    using const_pointer          = const char *;
    using reference              = value_type &;
    using const_reference        = const value_type &;
    using const_iterator         = const value_type *;
    using iterator               = const_iterator;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reverse_iterator       = const_reverse_iterator;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;

public:
    constexpr stack_string() noexcept : _M_length(0) {
        _M_data[0] = '\0';
    }

    constexpr stack_string(std::string_view sv) noexcept : _M_length(std::min(sv.size(), _Nm)) {
        std::copy_n(sv.data(), _M_length, _M_data);
        _M_data[_M_length] = '\0';
    }

    constexpr stack_string(const char *ptr) noexcept : stack_string(std::string_view{ptr}) {}

    constexpr stack_string(const stack_string &other) = default;

    auto operator=(const stack_string &other) -> stack_string & = default;

public:
    constexpr auto c_str() const noexcept -> const_pointer {
        return _M_data;
    }

    constexpr auto data() const noexcept -> const_pointer {
        return _M_data;
    }

    constexpr auto length() const noexcept -> size_type {
        return _M_length;
    }

    constexpr auto size() const noexcept -> size_type {
        return _M_length;
    }

    static constexpr auto capacity() noexcept -> size_type {
        return _Nm;
    }

    constexpr auto empty() const noexcept -> bool {
        return _M_length == 0;
    }

    constexpr auto front() const noexcept -> const_reference {
        return _M_data[0];
    }

    constexpr auto back() const noexcept -> const_reference {
        return _M_data[_M_length - 1];
    }

    constexpr auto operator[](std::size_t pos) const noexcept -> const_reference {
        return _M_data[pos];
    }

    constexpr auto at(std::size_t pos) const -> const_reference {
        if (pos >= _M_length)
            throw std::out_of_range{"stack_string::at"};
        return _M_data[pos];
    }

    constexpr auto front() noexcept -> reference {
        return _M_data[0];
    }

    constexpr auto back() noexcept -> reference {
        return _M_data[_M_length - 1];
    }

    constexpr auto operator[](std::size_t pos) noexcept -> reference {
        return _M_data[pos];
    }

    constexpr auto at(std::size_t pos) -> reference {
        if (pos >= _M_length)
            throw std::out_of_range{"stack_string::at"};
        return _M_data[pos];
    }

    constexpr auto begin() const noexcept -> const_iterator {
        return _M_data;
    }

    constexpr auto end() const noexcept -> const_iterator {
        return _M_data + _M_length;
    }

    constexpr auto begin() noexcept -> iterator {
        return _M_data;
    }

    constexpr auto end() noexcept -> iterator {
        return _M_data + _M_length;
    }

    constexpr auto view() const noexcept -> cstring_view {
        return cstring_view{*this};
    }

    constexpr operator std::string_view() const noexcept {
        return std::string_view{_M_data, _M_length};
    }

    explicit constexpr operator std::string() const noexcept {
        return std::string{_M_data, _M_length};
    }

    constexpr auto resize(std::size_t new_size) noexcept -> void {
        _M_length          = std::min(new_size, _Nm);
        _M_data[_M_length] = '\0';
    }

    template <typename _Operation>
    constexpr auto resize_and_overwrite(std::size_t new_size, _Operation op) -> void {
        _M_length          = std::min(new_size, _Nm);
        _M_data[_M_length] = '\0';
        try { // user should not modify those members
            op(std::as_const(_M_data), std::as_const(_M_length));
        } catch (...) {
            _M_data[_M_length] = '\0';
            throw;
        }
    }

    constexpr auto clear() noexcept -> void {
        _M_length  = 0;
        _M_data[0] = '\0';
    }

    constexpr auto append(std::string_view sv) noexcept -> void {
        auto n = std::min(sv.size(), _Nm - _M_length);
        std::copy_n(sv.data(), n, _M_data + _M_length);
        _M_length += n;
        _M_data[_M_length] = '\0';
    }

    constexpr auto append(char c) noexcept -> void {
        if (_M_length < _Nm) {
            _M_data[_M_length++] = c;
            _M_data[_M_length]   = '\0';
        }
    }

    constexpr auto append(std::size_t n, char c) noexcept -> void {
        n = std::min(n, _Nm - _M_length);
        std::fill_n(_M_data + _M_length, n, c);
        _M_length += n;
        _M_data[_M_length] = '\0';
    }

    constexpr auto push_back(char c) noexcept -> void {
        this->append(c);
    }

    constexpr auto remove(std::size_t n) noexcept -> void {
        _M_length -= std::min(n, _M_length);
        _M_data[_M_length] = '\0';
    }

    constexpr auto pop_back() noexcept -> void {
        this->remove(1);
    }

    constexpr auto fill(std::size_t n, char c) noexcept -> void {
        n = std::min(n, _Nm);
        std::fill_n(_M_data, n, c);
        _M_length          = n;
        _M_data[_M_length] = '\0';
    }

    constexpr auto operator+=(std::string_view sv) noexcept -> stack_string & {
        this->append(sv);
        return *this;
    }

    constexpr auto operator+=(char c) noexcept -> stack_string & {
        this->append(c);
        return *this;
    }

    constexpr auto operator<=>(const stack_string &other) const noexcept -> std::strong_ordering {
        return std::string_view{*this} <=> std::string_view{other};
    }

    constexpr auto operator==(const stack_string &other) const noexcept -> bool {
        return std::string_view{*this} == std::string_view{other};
    }

private:
    std::size_t _M_length;
    char _M_data[_Nm + 1];
};

template <std::size_t _Nm>
stack_string(const char (&)[_Nm]) -> stack_string<_Nm - 1>;

template <std::size_t _Nm1, std::size_t _Nm2>
inline constexpr auto operator+(const stack_string<_Nm1> &lhs, const stack_string<_Nm2> &rhs)
    -> stack_string<_Nm1 + _Nm2> {
    stack_string<_Nm1 + _Nm2> ret;
    ret.append(lhs);
    ret.append(rhs);
    return ret;
}

template <std::size_t _Nm>
inline constexpr auto operator+(const stack_string<_Nm> &lhs, std::string_view rhs) -> std::string {
    std::string ret{};
    ret.reserve(lhs.size() + rhs.size());
    ret.append(lhs.data(), lhs.size());
    ret.append(rhs.data(), rhs.size());
    return ret;
}

template <std::size_t _Nm>
inline constexpr auto operator+(std::string_view lhs, const stack_string<_Nm> &rhs) -> std::string {
    return rhs + lhs;
}

} // namespace dark
