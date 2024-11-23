#pragma once

#include <cstdint>
#include <immintrin.h>

namespace violet
{
struct simd
{
public:
    template <typename T>
    struct alignas(16) convert;

    template <>
    struct alignas(16) convert<float>
    {
        union
        {
            float t[4];
            __m128 v;
        };

        inline operator __m128() const
        {
            return v;
        }
    };

    template <>
    struct alignas(16) convert<std::uint32_t>
    {
        union
        {
            std::uint32_t t[4];
            __m128 v;
        };

        inline operator __m128() const
        {
            return v;
        }
    };

    template <std::uint32_t C1, std::uint32_t C2, std::uint32_t C3, std::uint32_t C4>
    struct mask
    {
        static_assert(C1 < 2 && C2 < 2 && C3 < 2 && C4 < 2);
        static constexpr convert<uint32_t> value =
            {0xFFFFFFFF * C1, 0xFFFFFFFF * C2, 0xFFFFFFFF * C3, 0xFFFFFFFF * C4};
    };

    template <std::uint32_t C1, std::uint32_t C2, std::uint32_t C3, std::uint32_t C4>
    static constexpr auto mask_v = mask<C1, C2, C3, C4>::value;

    template <std::uint32_t C1, std::uint32_t C2, std::uint32_t C3, std::uint32_t C4>
    struct shuffle_control
    {
        static_assert(C1 < 4 && C2 < 4 && C3 < 4 && C4 < 4);
        static constexpr std::uint32_t value = (C4 << 6) | (C3 << 4) | (C2 << 2) | C1;
    };

    template <std::uint32_t C1, std::uint32_t C2, std::uint32_t C3, std::uint32_t C4>
    static constexpr auto shuffle_control_v = shuffle_control<C1, C2, C3, C4>::value;

    static constexpr convert<float> one = {1.0f, 1.0f, 1.0f, 1.0f};

public:
    template <std::uint32_t C1, std::uint32_t C2, std::uint32_t C3, std::uint32_t C4>
    [[nodiscard]] static inline __m128 shuffle(__m128 a, __m128 b)
    {
        if constexpr (C1 == 0 && C2 == 1 && C3 == 0 && C4 == 1)
            return _mm_movelh_ps(a, b);
        else if constexpr (C1 == 2 && C2 == 3 && C3 == 2 && C4 == 3)
            return _mm_movehl_ps(b, a);
        else
            return _mm_shuffle_ps(a, b, shuffle_control_v<C1, C2, C3, C4>);
    }

    template <std::uint32_t C1, std::uint32_t C2, std::uint32_t C3, std::uint32_t C4>
    [[nodiscard]] static inline __m128 shuffle(__m128 v)
    {
        return shuffle<C1, C2, C3, C4>(v, v);
    }

    template <std::uint32_t C>
    [[nodiscard]] static inline __m128 replicate(__m128 v)
    {
        return shuffle<C, C, C, C>(v);
    }

// Undefining min, max macro with Windows.
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
    [[nodiscard]] static inline __m128 min(__m128 a, __m128 b)
    {
        return _mm_min_ps(a, b);
    }
    [[nodiscard]] static inline __m128 max(__m128 a, __m128 b)
    {
        return _mm_max_ps(a, b);
    }
#pragma pop_macro("min")
#pragma pop_macro("max")
};
} // namespace violet