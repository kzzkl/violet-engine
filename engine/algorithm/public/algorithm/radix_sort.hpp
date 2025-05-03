#pragma once

#include <span>
#include <vector>

namespace violet
{
template <typename Iter, typename KeyFunctor, typename CountType = std::size_t>
void radix_sort_32(Iter begin, Iter end, KeyFunctor&& key_functor)
{
    using count_type = CountType;
    using value_type = typename Iter::value_type;

    std::array<count_type, 1024 + 2048 + 2048> buckets = {};
    std::span<count_type> bucket0 = std::span<count_type>(buckets.data(), 1024);
    std::span<count_type> bucket1 = std::span<count_type>(bucket0.data() + 1024, 2048);
    std::span<count_type> bucket2 = std::span<count_type>(bucket1.data() + 2048, 2048);

    for (auto iter = begin; iter != end; ++iter)
    {
        std::uint32_t key = key_functor(*iter);
        ++bucket0[key & 1023];
        ++bucket1[(key >> 10) & 2047];
        ++bucket2[(key >> 21) & 2047];
    }

    count_type sum = 0;
    for (count_type& count : bucket0)
    {
        count_type t = count + sum;
        count = sum;
        sum = t;
    }

    sum = 0;
    for (count_type& count : bucket1)
    {
        count_type t = count + sum;
        count = sum;
        sum = t;
    }

    sum = 0;
    for (count_type& count : bucket2)
    {
        count_type t = count + sum;
        count = sum;
        sum = t;
    }

    std::vector<value_type> temp(end - begin);

    std::span<value_type> src = std::span<value_type>(begin, end);
    std::span<value_type> dst = std::span<value_type>(temp);
    for (count_type i = 0; i < temp.size(); ++i)
    {
        std::uint32_t key = key_functor(src[i]);
        dst[bucket0[key & 1023]++] = src[i];
    }

    std::swap(src, dst);
    for (count_type i = 0; i < temp.size(); ++i)
    {
        std::uint32_t key = key_functor(src[i]);
        dst[bucket1[(key >> 10) & 2047]++] = src[i];
    }

    std::swap(src, dst);
    for (count_type i = 0; i < temp.size(); ++i)
    {
        std::uint32_t key = key_functor(src[i]);
        dst[bucket2[(key >> 21) & 2047]++] = src[i];
    }

    std::copy(dst.begin(), dst.end(), begin);
}

template <typename Iter, typename CountType = std::size_t>
void radix_sort_32(Iter begin, Iter end)
{
    radix_sort_32(
        begin,
        end,
        [](const auto& value)
        {
            return value;
        });
}
} // namespace violet