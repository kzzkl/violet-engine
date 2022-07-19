#pragma once

#include "type.hpp"
#include <immintrin.h>

namespace ash::math
{
using int4_simd = __m128i;
using float4_simd = __m128;

struct alignas(16) float4x4_simd
{
    using row_type = float4_simd;

    inline row_type& operator[](std::size_t index) { return row[index]; }
    inline const row_type& operator[](std::size_t index) const { return row[index]; }

    row_type row[4];
};

struct simd
{
public:
    template <std::uint32_t I>
    static inline float get(float4_simd v)
    {
        if constexpr (I == 0)
        {
            return _mm_cvtss_f32(v);
        }
        else
        {
            float4_simd temp = shuffle<I, I, I, I>(v);
            return _mm_cvtss_f32(temp);
        }
    }

    static inline float4_simd set(float v) { return _mm_set_ps1(v); }
    static inline float4_simd set(float x, float y, float z, float w)
    {
        return _mm_setr_ps(x, y, z, w);
    }
    static inline float4_simd set(
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t z,
        std::uint32_t w)
    {
        union {
            __m128 v;
            __m128i i;
        };
        i = _mm_setr_epi32(x, y, z, w);
        return v;
    }

    static inline float4x4_simd set(
        float m11,
        float m12,
        float m13,
        float m14,
        float m21,
        float m22,
        float m23,
        float m24,
        float m31,
        float m32,
        float m33,
        float m34,
        float m41,
        float m42,
        float m43,
        float m44)
    {
        return float4x4_simd{
            set(m11, m12, m13, m14),
            set(m21, m22, m23, m24),
            set(m31, m32, m33, m34),
            set(m41, m42, m43, m44)};
    }

    template <std::uint32_t Mask>
    static inline float4_simd mask()
    {
        static const float4_simd value = make_mask(Mask);
        return value;
    }

    template <std::uint32_t C1, std::uint32_t C2, std::uint32_t C3, std::uint32_t C4>
    static inline float4_simd shuffle(float4_simd a, float4_simd b)
    {
        return _mm_shuffle_ps(a, b, shuffle_control<C1, C2, C3, C4>::value);
    }

    template <std::uint32_t C1, std::uint32_t C2, std::uint32_t C3, std::uint32_t C4>
    static inline float4_simd shuffle(float4_simd v)
    {
        return shuffle<C1, C2, C3, C4>(v, v);
    }

    template <std::uint32_t C>
    static inline float4_simd replicate(float4_simd v)
    {
        return shuffle<C, C, C, C>(v);
    }

// Undefining min, max macro with Windows.
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
    static inline float4_simd min(float4_simd a, float4_simd b)
    {
        return _mm_min_ps(a, b);
    }
    static inline float4_simd max(float4_simd a, float4_simd b)
    {
        return _mm_max_ps(a, b);
    }
#pragma pop_macro("min")
#pragma pop_macro("max")

    static inline float4_simd load(const float3& v)
    {
        __m128 x = _mm_load_ss(&v[0]);
        __m128 y = _mm_load_ss(&v[1]);
        __m128 z = _mm_load_ss(&v[2]);
        __m128 xy = _mm_unpacklo_ps(x, y);
        return _mm_movelh_ps(xy, z);
    }

    static inline float4_simd load(const float3& v, float w)
    {
        __m128 t1 = _mm_load_ss(&v[0]);
        __m128 t2 = _mm_load_ss(&v[1]);
        __m128 t3 = _mm_load_ss(&v[2]);
        __m128 t4 = _mm_load_ss(&w);
        t1 = _mm_unpacklo_ps(t1, t2);
        t3 = _mm_unpacklo_ps(t3, t4);
        return _mm_movelh_ps(t1, t3);
    }

    static inline float4_simd load(const float4& v)
    {
        return _mm_loadu_ps(&v[0]);
    }

    static inline float4_simd load(const float4_align& v)
    {
        return _mm_load_ps(&v[0]);
    }

    static inline float4x4_simd load(const float4x4& m)
    {
        return {
            _mm_loadu_ps(&m[0][0]),
            _mm_loadu_ps(&m[1][0]),
            _mm_loadu_ps(&m[2][0]),
            _mm_loadu_ps(&m[3][0])};
    }

    static inline float4x4_simd load(const float4x4_align& m)
    {
        return {
            _mm_load_ps(&m[0][0]),
            _mm_load_ps(&m[1][0]),
            _mm_load_ps(&m[2][0]),
            _mm_load_ps(&m[3][0])};
    }

    static inline void store(float4_simd source, float3& destination)
    {
        __m128 y = replicate<1>(source);
        __m128 z = replicate<2>(source);

        _mm_store_ss(&destination[0], source);
        _mm_store_ss(&destination[1], y);
        _mm_store_ss(&destination[2], z);
    }

    static inline void store(float4_simd source, float4& destination)
    {
        _mm_storeu_ps(&destination[0], source);
    }

    static inline void store(float4_simd source, float4_align& destination)
    {
        _mm_store_ps(&destination[0], source);
    }

    static inline void store(const float4x4_simd& source, float4x3& destination)
    {
        _mm_storeu_ps(&destination[0][0], source[0]);
        _mm_storeu_ps(&destination[1][0], source[1]);
        _mm_storeu_ps(&destination[2][0], source[2]);
    }

    static inline void store(const float4x4_simd& source, float4x3_align& destination)
    {
        _mm_store_ps(&destination[0][0], source[0]);
        _mm_store_ps(&destination[1][0], source[1]);
        _mm_store_ps(&destination[2][0], source[2]);
    }

    static inline void store(const float4x4_simd& source, float4x4& destination)
    {
        _mm_storeu_ps(&destination[0][0], source[0]);
        _mm_storeu_ps(&destination[1][0], source[1]);
        _mm_storeu_ps(&destination[2][0], source[2]);
        _mm_storeu_ps(&destination[3][0], source[3]);
    }

    static inline void store(const float4x4_simd& source, float4x4_align& destination)
    {
        _mm_store_ps(&destination[0][0], source[0]);
        _mm_store_ps(&destination[1][0], source[1]);
        _mm_store_ps(&destination[2][0], source[2]);
        _mm_store_ps(&destination[3][0], source[3]);
    }

    template <std::uint32_t I>
    static inline float4_simd identity_row()
    {
        if constexpr (I == 0)
        {
            static const float4_simd result = set(1.0f, 0.0f, 0.0f, 0.0f);
            return result;
        }
        else if constexpr (I == 1)
        {
            static const float4_simd result = set(0.0f, 1.0f, 0.0f, 0.0f);
            return result;
        }
        else if constexpr (I == 2)
        {
            static const float4_simd result = set(0.0f, 0.0f, 1.0f, 0.0f);
            return result;
        }
        else if constexpr (I == 3)
        {
            static const float4_simd result = set(0.0f, 0.0f, 0.0f, 1.0f);
            return result;
        }
    }

private:
    template <std::uint32_t C1, std::uint32_t C2, std::uint32_t C3, std::uint32_t C4>
    struct shuffle_control
    {
        static constexpr std::uint32_t value = (C4 << 6) | (C3 << 4) | (C2 << 2) | C1;
    };

    static inline float4_simd make_mask(std::uint32_t mask)
    {
        std::uint32_t x = (mask & 0x1000) == 0x1000 ? 0xFFFFFFFF : 0x00000000;
        std::uint32_t y = (mask & 0x0100) == 0x0100 ? 0xFFFFFFFF : 0x00000000;
        std::uint32_t z = (mask & 0x0010) == 0x0010 ? 0xFFFFFFFF : 0x00000000;
        std::uint32_t w = (mask & 0x0001) == 0x0001 ? 0xFFFFFFFF : 0x00000000;
        return set(x, y, z, w);
    }
};
} // namespace ash::math