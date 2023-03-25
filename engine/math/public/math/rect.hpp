#pragma once

namespace violet
{
template <typename T>
struct rect
{
    using value_type = T;

    value_type x;
    value_type y;

    value_type width;
    value_type height;

    void contain(T px, T py) const noexcept
    {
        return x <= px && x + width >= px && y <= py && y + height >= py;
    }
};
} // namespace violet