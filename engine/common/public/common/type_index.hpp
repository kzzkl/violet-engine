#pragma once

#include <type_traits>

namespace violet
{
template <typename T, typename Index, Index Initial = 0>
class type_index
{
public:
    using index_type = Index;
    using self_type = type_index<T, Index, Initial>;

    template <typename R>
    static const index_type value() noexcept
    {
        return value_impl<std::remove_const_t<R>>();
    }

private:
    template <typename R>
    static const index_type value_impl() noexcept
    {
        static const index_type index = self_type::next();
        return index;
    }

    static index_type next() noexcept
    {
        static index_type next = Initial;
        return next++;
    }
};
} // namespace violet