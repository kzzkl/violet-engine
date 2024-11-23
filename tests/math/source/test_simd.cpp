#include "test_common.hpp"

namespace violet::test
{
TEST_CASE("simd::shuffle", "")
{
    auto v = vector::set(1.0f, 2.0f, 3.0f, 4.0f);
    auto v2 = simd::shuffle<1, 1, 2, 2>(v);

    vec4f result;
    math::store(v2, result);

    CHECK(equal(result, vec4f{2.0f, 2.0f, 3.0f, 3.0f}));
}

TEST_CASE("simd::store", "")
{
    vec4f_simd v = vector::set(1.0f, 2.0f, 3.0f, 4.0f);
    vec4f r4;
    math::store(v, r4);

    CHECK(equal(r4, vec4f{1.0f, 2.0f, 3.0f, 4.0f}));

    vec3f r3;
    math::store(v, r3);
    CHECK(equal(r3, vec3f{1.0f, 2.0f, 3.0f}));
}
} // namespace violet::test