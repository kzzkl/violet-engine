#pragma once

#include <string>
#include <unordered_map>

namespace violet
{
namespace detail
{
struct string_hash
{
    using is_transparent = void;

    size_t operator()(const std::string& key) const
    {
        return std::hash<std::string>{}(key);
    }

    size_t operator()(std::string_view sv) const noexcept
    {
        return std::hash<std::string_view>{}(sv);
    }
};

struct string_equal
{
    using is_transparent = void;
    bool operator()(std::string_view a, std::string_view b) const noexcept
    {
        return a == b;
    }
};
} // namespace detail

template <typename T>
using string_map = std::unordered_map<std::string, T, detail::string_hash, detail::string_equal>;
} // namespace violet