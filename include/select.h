#pragma once
#include "socket.h"

namespace dark {

namespace __detail {

template <bool _Empty = false>
struct SocketList;

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

template <int _Tag>
struct FileSet<false, _Tag> {
public:
    FileSet(std::initializer_list<SocketRef> files) noexcept {
        FD_ZERO(&_M_set);
        for (const auto &file : files)
            FD_SET(file.get()._M_file.unsafe_get(), &_M_set);
    }

    auto contains(const Socket &file) const noexcept -> bool {
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
