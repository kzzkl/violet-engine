#include "math/types.hpp"
#include "test_common.hpp"
#include <type_traits>

namespace violet::test
{
// Copy and move constructors are implicitly defaulted, so the vector types must stay trivially
// copy constructible.
static_assert(std::is_trivially_copy_constructible_v<vec2f>);
static_assert(std::is_trivially_copy_constructible_v<vec3f>);
static_assert(std::is_trivially_copy_constructible_v<vec4f>);
static_assert(std::is_trivially_copy_constructible_v<vec2i>);
static_assert(std::is_trivially_copy_constructible_v<vec3u>);
static_assert(std::is_trivially_copy_constructible_v<vec4f_simd>);

// ==================== Default Constructor Tests ====================

TEST_CASE("vec2 default constructor", "[types]")
{
    vec2f f;
    CHECK(equal(f, vec2f{0.0f, 0.0f}));

    vec2i i;
    CHECK(i.x == 0);
    CHECK(i.y == 0);

    vec2u u;
    CHECK(u.x == 0u);
    CHECK(u.y == 0u);
}

TEST_CASE("vec3 default constructor", "[types]")
{
    vec3f f;
    CHECK(equal(f, vec3f{0.0f, 0.0f, 0.0f}));

    vec3i i;
    CHECK(i.x == 0);
    CHECK(i.y == 0);
    CHECK(i.z == 0);
}

TEST_CASE("vec4 default constructor", "[types]")
{
    vec4f f;
    CHECK(equal(f, vec4f{0.0f, 0.0f, 0.0f, 0.0f}));

    vec4i i;
    CHECK(i.x == 0);
    CHECK(i.y == 0);
    CHECK(i.z == 0);
    CHECK(i.w == 0);

    vec4u u;
    CHECK(u.x == 0u);
    CHECK(u.y == 0u);
    CHECK(u.z == 0u);
    CHECK(u.w == 0u);
}

// ==================== Value Constructor Tests ====================

TEST_CASE("vec2 value constructor", "[types]")
{
    CHECK(equal(vec2f{2.5f}, vec2f{2.5f, 2.5f}));
    CHECK(equal(vec2f{-3.0f}, vec2f{-3.0f, -3.0f}));
    CHECK(equal(vec2f{0.0f}, vec2f{0.0f, 0.0f}));

    vec2f parens(1.5f);
    CHECK(equal(parens, vec2f{1.5f, 1.5f}));

    // The value constructor is implicit, so a scalar converts to a broadcast vector.
    vec2f implicit = 4.0f;
    CHECK(equal(implicit, vec2f{4.0f, 4.0f}));

    vec2i i{7};
    CHECK(i.x == 7);
    CHECK(i.y == 7);
}

TEST_CASE("vec3 value constructor", "[types]")
{
    CHECK(equal(vec3f{2.0f}, vec3f{2.0f, 2.0f, 2.0f}));
    CHECK(equal(vec3f{-1.25f}, vec3f{-1.25f, -1.25f, -1.25f}));
    CHECK(equal(vec3f{0.0f}, vec3f{0.0f, 0.0f, 0.0f}));

    vec3f parens(3.0f);
    CHECK(equal(parens, vec3f{3.0f, 3.0f, 3.0f}));

    vec3f implicit = 1.0f;
    CHECK(equal(implicit, vec3f{1.0f, 1.0f, 1.0f}));

    vec3i i{5};
    CHECK(i.x == 5);
    CHECK(i.y == 5);
    CHECK(i.z == 5);
}

TEST_CASE("vec4 value constructor", "[types]")
{
    CHECK(equal(vec4f{3.0f}, vec4f{3.0f, 3.0f, 3.0f, 3.0f}));
    CHECK(equal(vec4f{-0.5f}, vec4f{-0.5f, -0.5f, -0.5f, -0.5f}));
    CHECK(equal(vec4f{0.0f}, vec4f{0.0f, 0.0f, 0.0f, 0.0f}));

    vec4f parens(1.0f);
    CHECK(equal(parens, vec4f{1.0f, 1.0f, 1.0f, 1.0f}));

    vec4f implicit = 2.0f;
    CHECK(equal(implicit, vec4f{2.0f, 2.0f, 2.0f, 2.0f}));

    vec4i i{9};
    CHECK(i.x == 9);
    CHECK(i.y == 9);
    CHECK(i.z == 9);
    CHECK(i.w == 9);
}

// ==================== Component Constructor Tests ====================

TEST_CASE("vec2 component constructor", "[types]")
{
    vec2f braces{1.0f, 2.0f};
    CHECK(braces.x == 1.0f);
    CHECK(braces.y == 2.0f);

    vec2f parens(3.0f, 4.0f);
    CHECK(parens.x == 3.0f);
    CHECK(parens.y == 4.0f);

    vec2f copy_list = {5.0f, 6.0f};
    CHECK(copy_list.x == 5.0f);
    CHECK(copy_list.y == 6.0f);

    vec2f mixed{-1.0f, 2.0f};
    CHECK(mixed.x == -1.0f);
    CHECK(mixed.y == 2.0f);

    vec2i i{-1, 2};
    CHECK(i.x == -1);
    CHECK(i.y == 2);

    vec2u u{1u, 2u};
    CHECK(u.x == 1u);
    CHECK(u.y == 2u);
}

TEST_CASE("vec3 component constructor", "[types]")
{
    vec3f braces{1.0f, 2.0f, 3.0f};
    CHECK(braces.x == 1.0f);
    CHECK(braces.y == 2.0f);
    CHECK(braces.z == 3.0f);

    vec3f parens(4.0f, 5.0f, 6.0f);
    CHECK(parens.x == 4.0f);
    CHECK(parens.y == 5.0f);
    CHECK(parens.z == 6.0f);

    vec3f copy_list = {7.0f, 8.0f, 9.0f};
    CHECK(copy_list.x == 7.0f);
    CHECK(copy_list.y == 8.0f);
    CHECK(copy_list.z == 9.0f);

    vec3i i{-1, 0, 1};
    CHECK(i.x == -1);
    CHECK(i.y == 0);
    CHECK(i.z == 1);

    vec3u u{1u, 2u, 3u};
    CHECK(u.x == 1u);
    CHECK(u.y == 2u);
    CHECK(u.z == 3u);
}

TEST_CASE("vec4 component constructor", "[types]")
{
    vec4f braces{1.0f, 2.0f, 3.0f, 4.0f};
    CHECK(braces.x == 1.0f);
    CHECK(braces.y == 2.0f);
    CHECK(braces.z == 3.0f);
    CHECK(braces.w == 4.0f);

    vec4f parens(5.0f, 6.0f, 7.0f, 8.0f);
    CHECK(parens.x == 5.0f);
    CHECK(parens.y == 6.0f);
    CHECK(parens.z == 7.0f);
    CHECK(parens.w == 8.0f);

    vec4f copy_list = {9.0f, 10.0f, 11.0f, 12.0f};
    CHECK(copy_list.x == 9.0f);
    CHECK(copy_list.y == 10.0f);
    CHECK(copy_list.z == 11.0f);
    CHECK(copy_list.w == 12.0f);

    vec4i i{-1, 0, 1, 2};
    CHECK(i.x == -1);
    CHECK(i.y == 0);
    CHECK(i.z == 1);
    CHECK(i.w == 2);

    vec4u u{1u, 2u, 3u, 4u};
    CHECK(u.x == 1u);
    CHECK(u.y == 2u);
    CHECK(u.z == 3u);
    CHECK(u.w == 4u);
}

// ==================== Copy Constructor Tests ====================

TEST_CASE("vec2 copy constructor", "[types]")
{
    const vec2f source{1.0f, 2.0f};

    vec2f direct{source};
    CHECK(direct.x == source.x);
    CHECK(direct.y == source.y);

    vec2f copy_list = source;
    CHECK(copy_list.x == source.x);
    CHECK(copy_list.y == source.y);

    // The copy is independent from the source.
    direct.x = 5.0f;
    CHECK(source.x == 1.0f);

    vec2i source_i{-1, 2};
    vec2i copy_i{source_i};
    CHECK(copy_i.x == -1);
    CHECK(copy_i.y == 2);
}

TEST_CASE("vec3 copy constructor", "[types]")
{
    const vec3f source{1.0f, 2.0f, 3.0f};

    vec3f direct{source};
    CHECK(direct.x == source.x);
    CHECK(direct.y == source.y);
    CHECK(direct.z == source.z);

    vec3f copy_list = source;
    CHECK(copy_list.x == source.x);
    CHECK(copy_list.y == source.y);
    CHECK(copy_list.z == source.z);

    direct.z = 5.0f;
    CHECK(source.z == 3.0f);

    vec3i source_i{-1, 0, 1};
    vec3i copy_i{source_i};
    CHECK(copy_i.x == -1);
    CHECK(copy_i.y == 0);
    CHECK(copy_i.z == 1);
}

TEST_CASE("vec4 copy constructor", "[types]")
{
    const vec4f source{1.0f, 2.0f, 3.0f, 4.0f};

    vec4f direct{source};
    CHECK(direct.x == source.x);
    CHECK(direct.y == source.y);
    CHECK(direct.z == source.z);
    CHECK(direct.w == source.w);

    vec4f copy_list = source;
    CHECK(copy_list.x == source.x);
    CHECK(copy_list.y == source.y);
    CHECK(copy_list.z == source.z);
    CHECK(copy_list.w == source.w);

    direct.w = 5.0f;
    CHECK(source.w == 4.0f);

    vec4i source_i{-1, 0, 1, 2};
    vec4i copy_i{source_i};
    CHECK(copy_i.x == -1);
    CHECK(copy_i.y == 0);
    CHECK(copy_i.z == 1);
    CHECK(copy_i.w == 2);
}

// ==================== Conversion Constructor Tests ====================

TEST_CASE("vec2 conversion constructor", "[types]")
{
    vec3f from_vec3{1.0f, 2.0f, 3.0f};
    vec2f xy = from_vec3;
    CHECK(xy.x == 1.0f);
    CHECK(xy.y == 2.0f);

    vec4f from_vec4{4.0f, 5.0f, 6.0f, 7.0f};
    vec2f xy_from_vec4 = from_vec4;
    CHECK(xy_from_vec4.x == 4.0f);
    CHECK(xy_from_vec4.y == 5.0f);

    // Element type conversion, e.g. vec2i -> vec2f.
    vec2i ints{1, 2};
    vec2f floats = ints;
    CHECK(floats.x == 1.0f);
    CHECK(floats.y == 2.0f);

    // Narrowing conversion truncates toward zero.
    vec2f trunc_source{1.75f, -2.75f};
    vec2i truncated = trunc_source;
    CHECK(truncated.x == 1);
    CHECK(truncated.y == -2);
}

TEST_CASE("vec3 conversion constructor", "[types]")
{
    vec4f from_vec4{1.0f, 2.0f, 3.0f, 4.0f};
    vec3f xyz = from_vec4;
    CHECK(xyz.x == 1.0f);
    CHECK(xyz.y == 2.0f);
    CHECK(xyz.z == 3.0f);

    vec3i ints{1, 2, 3};
    vec3f floats = ints;
    CHECK(floats.x == 1.0f);
    CHECK(floats.y == 2.0f);
    CHECK(floats.z == 3.0f);

    vec3f trunc_source{1.5f, -1.5f, 2.25f};
    vec3i truncated = trunc_source;
    CHECK(truncated.x == 1);
    CHECK(truncated.y == -1);
    CHECK(truncated.z == 2);
}

TEST_CASE("vec4 conversion constructor", "[types]")
{
    vec4i ints{1, 2, 3, 4};
    vec4f floats = ints;
    CHECK(floats.x == 1.0f);
    CHECK(floats.y == 2.0f);
    CHECK(floats.z == 3.0f);
    CHECK(floats.w == 4.0f);

    vec4f trunc_source{-0.5f, 0.5f, 1.5f, 2.5f};
    vec4i truncated = trunc_source;
    CHECK(truncated.x == 0);
    CHECK(truncated.y == 0);
    CHECK(truncated.z == 1);
    CHECK(truncated.w == 2);
}

TEST_CASE("vec4f_simd constructor", "[types]")
{
    vec4f_simd from_register = _mm_set_ps(4.0f, 3.0f, 2.0f, 1.0f);
    CHECK(equal(from_register, vec4f{1.0f, 2.0f, 3.0f, 4.0f}));

    __m128 raw = from_register;
    vec4f_simd round_trip{raw};
    CHECK(equal(round_trip, vec4f{1.0f, 2.0f, 3.0f, 4.0f}));
}

// ==================== Compile-time Tests ====================

TEST_CASE("vector constructor constexpr", "[types]")
{
    constexpr vec2f v2{1.0f, 2.0f};
    static_assert(v2.x == 1.0f && v2.y == 2.0f);

    constexpr vec3f v3{3.0f};
    static_assert(v3.x == 3.0f && v3.y == 3.0f && v3.z == 3.0f);

    constexpr vec4i v4{-1, -2, -3, -4};
    static_assert(v4.x == -1 && v4.w == -4);

    constexpr vec4f v4f;
    static_assert(v4f.x == 0.0f && v4f.y == 0.0f && v4f.z == 0.0f && v4f.w == 0.0f);

    CHECK(equal(v2, vec2f{1.0f, 2.0f}));
    CHECK(equal(v3, vec3f{3.0f, 3.0f, 3.0f}));
    CHECK(v4.w == -4);
}
} // namespace violet::test
