#pragma once

namespace ash
{
template <typename T, typename Index, Index Initial = 0>
class index_generator
{
public:
    using index_type = Index;
    using self_type = index_generator<T, Index, Initial>;

    template <typename R>
    static const index_type value() noexcept
    {
        static const index_type index = self_type::next();
        return index;
    }

private:
    static index_type next() noexcept
    {
        static index_type next = Initial;
        return next++;
    }
};
} // namespace ash