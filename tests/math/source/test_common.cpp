#include "test_common.hpp"

#define VALUE_WITH_MARGIN(v) Catch::Approx(v).margin(0.0005)

namespace violet::test
{
bool equal(float a, float b)
{
    return VALUE_WITH_MARGIN(a) == b;
}

bool equal(const violet::vec2f& a, const violet::vec2f& b)
{
    for (std::size_t i = 0; i < 2; ++i)
    {
        if (VALUE_WITH_MARGIN(a[i]) != b[i])
            return false;
    }
    return true;
}

bool equal(const violet::vec3f& a, const violet::vec3f& b)
{
    for (std::size_t i = 0; i < 3; ++i)
    {
        if (VALUE_WITH_MARGIN(a[i]) != b[i])
            return false;
    }
    return true;
}

bool equal(const violet::vec4f& a, const violet::vec4f& b)
{
    for (std::size_t i = 0; i < 4; ++i)
    {
        if (VALUE_WITH_MARGIN(a[i]) != b[i])
            return false;
    }
    return true;
}

bool equal(const violet::mat4f& a, const violet::mat4f& b)
{
    for (std::size_t i = 0; i < 4; ++i)
    {
        for (std::size_t j = 0; j < 4; ++j)
        {
            if (VALUE_WITH_MARGIN(a[i][j]) != b[i][j])
                return false;
        }
    }
    return true;
}

bool equal(const violet::vec4f_simd& a, const violet::vec3f& b)
{
    vec3f va;
    math::store(a, va);

    return equal(va, b);
}

bool equal(const violet::vec4f_simd& a, const violet::vec4f& b)
{
    vec4f va;
    math::store(a, va);

    return equal(va, b);
}

bool equal(const violet::mat4f_simd& a, const violet::mat4f& b)
{
    mat4f ma;
    math::store(a, ma);

    return equal(ma, b);
}
} // namespace violet::test