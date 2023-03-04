#pragma once

namespace violet::math
{
template <typename T>
struct rect
{
    using value_type = T;

    value_type x;
    value_type y;

    value_type width;
    value_type height;

    template <typename T>
    void contain(T px, T py) const noexcept
    {
        return x <= px && x + width >= px && y <= py && y + height >= py;
    }
};
} // namespace violet::math