#pragma once
#include <cstddef>
#include <string>
#include <string_view>

namespace dark {

struct cstring_view {
private:
    using _Sv_t = std::string_view;
    struct _Unsafe_t {};
    constexpr cstring_view(const char *data, std::size_t length, _Unsafe_t) noexcept :
        _M_length{length}, _M_data{data} {}

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
    static constexpr std::size_t npos = std::string::npos;
    constexpr cstring_view() noexcept : _M_length{}, _M_data{} {}
    constexpr cstring_view(const char *data) noexcept :
        _M_length{std::char_traits<char>::length(data)}, _M_data{data} {}
    constexpr cstring_view(const std::string &str) noexcept :
        _M_length{str.length()}, _M_data{str.data()} {}
    constexpr cstring_view(const char *data, std::size_t length) noexcept :
        _M_length{length}, _M_data{data} {
        if (data[length] != '\0')
            *this = cstring_view{data};
    }

    constexpr operator std::string_view() const noexcept {
        return _Sv_t{_M_data, _M_length};
    }
    explicit constexpr operator std::string() const noexcept {
        return std::string{_M_data, _M_length};
    }

    constexpr auto c_str() const noexcept {
        return _M_data;
    }
    constexpr auto data() const noexcept {
        return _M_data;
    }
    constexpr auto length() const noexcept {
        return _M_length;
    }
    constexpr auto size() const noexcept {
        return _M_length;
    }
    constexpr auto empty() const noexcept {
        return _M_length == 0;
    }
    constexpr auto front() const noexcept {
        return _M_data[0];
    }
    constexpr auto back() const noexcept {
        return _M_data[_M_length - 1];
    }
    constexpr auto operator[](std::size_t pos) const noexcept {
        return _M_data[pos];
    }
    constexpr auto begin() const noexcept {
        return _M_data;
    }
    constexpr auto end() const noexcept {
        return _M_data + _M_length;
    }

    // Remark: cstring must be null-terminated
    constexpr auto substr(std::size_t pos) const noexcept {
        return cstring_view{_M_data + pos, _M_length - pos, {}};
    }

    constexpr auto split(std::size_t pos) const noexcept -> std::pair<_Sv_t, cstring_view> {
        return {_Sv_t{_M_data, pos}, cstring_view{_M_data + pos, _M_length - pos, {}}};
    }

    constexpr auto pop_front(std::size_t n = 1) noexcept -> _Sv_t {
        auto sv = _Sv_t{_M_data, n};
        _M_data += n;
        _M_length -= n;
        return sv;
    }

    constexpr auto find(cstring_view sv, std::size_t pos = 0) const noexcept {
        return _Sv_t{*this}.find(sv, pos);
    }

    constexpr auto rfind(cstring_view sv, std::size_t pos = std::string::npos) const noexcept {
        return _Sv_t{*this}.rfind(sv, pos);
    }

    constexpr auto find_first_of(cstring_view sv, std::size_t pos = 0) const noexcept {
        return _Sv_t{*this}.find_first_of(sv, pos);
    }

    constexpr auto
    find_last_of(cstring_view sv, std::size_t pos = std::string::npos) const noexcept {
        return _Sv_t{*this}.find_last_of(sv, pos);
    }

    constexpr auto find_first_not_of(cstring_view sv, std::size_t pos = 0) const noexcept {
        return _Sv_t{*this}.find_first_not_of(sv, pos);
    }

    constexpr auto
    find_last_not_of(cstring_view sv, std::size_t pos = std::string::npos) const noexcept {
        return _Sv_t{*this}.find_last_not_of(sv, pos);
    }

    template <typename... _Args>
        requires requires(_Sv_t sv, _Args &&...args) { sv.compare(std::forward<_Args>(args)...); }
    constexpr auto compare(_Args &&...args) const noexcept {
        return _Sv_t{*this}.compare(std::forward<_Args>(args)...);
    }

    template <typename... _Args>
        requires requires(_Sv_t sv, _Args &&...args) {
            sv.starts_with(std::forward<_Args>(args)...);
        }
    constexpr auto starts_with(_Args &&...args) const noexcept {
        return _Sv_t{*this}.starts_with(std::forward<_Args>(args)...);
    }

    template <typename... _Args>
        requires requires(_Sv_t sv, _Args &&...args) { sv.ends_with(std::forward<_Args>(args)...); }
    constexpr auto ends_with(_Args &&...args) const noexcept {
        return _Sv_t{*this}.ends_with(std::forward<_Args>(args)...);
    }

    constexpr auto operator<=>(cstring_view sv) const noexcept {
        return _Sv_t{*this} <=> _Sv_t{sv};
    }

    constexpr auto operator==(cstring_view sv) const noexcept {
        return _Sv_t{*this} == _Sv_t{sv};
    }

private:
    std::size_t _M_length;
    const char *_M_data;
};

} // namespace dark
