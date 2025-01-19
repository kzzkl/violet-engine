#pragma once

#include "math/simd.hpp"
#include <cstddef>
#include <initializer_list>

namespace violet
{
template <typename T>
struct vec2
{
    using value_type = T;
    using self_type = vec2<T>;

    union
    {
        struct
        {
            value_type x, y;
        };

        struct
        {
            value_type r, g;
        };

        value_type data[2];
    };

    [[nodiscard]] inline value_type& operator[](std::size_t index)
    {
        return data[index];
    }

    [[nodiscard]] inline value_type operator[](std::size_t index) const
    {
        return data[index];
    }

    [[nodiscard]] inline bool operator==(const self_type& other) const noexcept
    {
        return x == other.x && y == other.y;
    }
};

template <typename T>
struct vec3
{
    using value_type = T;
    using self_type = vec3<T>;

    union
    {
        struct
        {
            value_type x, y, z;
        };

        struct
        {
            value_type r, g, b;
        };

        value_type data[3];
    };

    [[nodiscard]] inline value_type& operator[](std::size_t index)
    {
        return data[index];
    }

    [[nodiscard]] inline const value_type& operator[](std::size_t index) const
    {
        return data[index];
    }

    [[nodiscard]] inline bool operator==(const self_type& other) const noexcept
    {
        return x == other.x && y == other.y && z == other.z;
    }
};

template <typename T>
struct vec4
{
    using value_type = T;
    using self_type = vec4<T>;

    union
    {
        struct
        {
            value_type x, y, z, w;
        };

        struct
        {
            value_type r, g, b, a;
        };

        value_type data[4];
    };

    [[nodiscard]] inline value_type& operator[](std::size_t index)
    {
        return data[index];
    }

    [[nodiscard]] inline const value_type& operator[](std::size_t index) const
    {
        return data[index];
    }

    [[nodiscard]] inline bool operator==(const self_type& other) const noexcept
    {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }
};

using vec2f = vec2<float>;
using vec3f = vec3<float>;
using vec4f = vec4<float>;

using vec2i = vec2<std::int32_t>;
using vec3i = vec3<std::int32_t>;
using vec4i = vec4<std::int32_t>;

using vec2u = vec2<std::uint32_t>;
using vec3u = vec3<std::uint32_t>;
using vec4u = vec4<std::uint32_t>;

template <>
struct vec4<simd>
{
    using value_type = float;

    vec4() noexcept = default;

    vec4(__m128 v) noexcept
        : m_v(v)
    {
    }

    operator __m128() const noexcept
    {
        return m_v;
    }

private:
    __m128 m_v;
};
using vec4f_simd = vec4<simd>;

template <typename T>
struct mat4x4
{
    using value_type = T;
    using row_type = vec4<T>;

    row_type row[4];

    mat4x4() noexcept
        : row{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}
    {
    }

    mat4x4(value_type value)
        : row{{value, value, value, value},
              {value, value, value, value},
              {value, value, value, value},
              {value, value, value, value}}
    {
    }

    mat4x4(const std::initializer_list<row_type>& rows)
    {
        std::size_t i = 0;
        for (auto& r : rows)
        {
            row[i++] = r;
        }
    }

    [[nodiscard]] inline row_type& operator[](std::size_t index)
    {
        return row[index];
    }

    [[nodiscard]] inline const row_type& operator[](std::size_t index) const
    {
        return row[index];
    }
};

template <>
struct mat4x4<simd>
{
    using value_type = float;
    using row_type = vec4f_simd;

    row_type row[4];

    [[nodiscard]] inline row_type& operator[](std::size_t index)
    {
        return row[index];
    }

    [[nodiscard]] inline const row_type& operator[](std::size_t index) const
    {
        return row[index];
    }
};

template <typename T>
using mat4 = mat4x4<T>;

using mat4x4f = mat4x4<float>;
using mat4x4f_simd = mat4x4<simd>;
using mat4f = mat4x4f;
using mat4f_simd = mat4x4f_simd;

namespace math
{
[[nodiscard]] static inline vec4f_simd load(vec2f v) noexcept
{
    return _mm_castpd_ps(_mm_load_sd(reinterpret_cast<const double*>(&v[0])));
}

[[nodiscard]] static inline vec4f_simd load(const vec3f& v) noexcept
{
    __m128 x = _mm_load_ss(&v[0]);
    __m128 y = _mm_load_ss(&v[1]);
    __m128 z = _mm_load_ss(&v[2]);
    __m128 xy = _mm_unpacklo_ps(x, y);
    return _mm_movelh_ps(xy, z);
}

[[nodiscard]] static inline vec4f_simd load(const vec3f& v, float w) noexcept
{
    __m128 t1 = _mm_load_ss(&v[0]);
    __m128 t2 = _mm_load_ss(&v[1]);
    __m128 t3 = _mm_load_ss(&v[2]);
    __m128 t4 = _mm_load_ss(&w);
    t1 = _mm_unpacklo_ps(t1, t2);
    t3 = _mm_unpacklo_ps(t3, t4);
    return _mm_movelh_ps(t1, t3);
}

[[nodiscard]] static inline vec4f_simd load(const vec4f& v) noexcept
{
    return _mm_loadu_ps(&v[0]);
}

[[nodiscard]] static inline mat4f_simd load(const mat4f& m) noexcept
{
    return {math::load(m[0]), math::load(m[1]), math::load(m[2]), math::load(m[3])};
}

static inline void store(vec4f_simd src, vec2f& dst) noexcept
{
    _mm_store_sd(reinterpret_cast<double*>(&dst[0]), _mm_castps_pd(src));
}

static inline void store(vec4f_simd src, vec3f& dst) noexcept
{
    __m128 y = simd::replicate<1>(src);
    __m128 z = simd::replicate<2>(src);

    _mm_store_ss(&dst.x, src);
    _mm_store_ss(&dst.y, y);
    _mm_store_ss(&dst.z, z);
}

static inline void store(vec4f_simd src, vec4f& dst) noexcept
{
    _mm_storeu_ps(&dst[0], src);
}

static inline void store(mat4f_simd src, mat4f& dst) noexcept
{
    math::store(src[0], dst[0]);
    math::store(src[1], dst[1]);
    math::store(src[2], dst[2]);
    math::store(src[3], dst[3]);
}
} // namespace math
} // namespace violet