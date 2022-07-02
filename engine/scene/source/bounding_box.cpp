#include "scene/bounding_box.hpp"
#include <limits>

namespace ash::scene
{
bounding_box::bounding_box() : m_aabb{}
{
}

void bounding_box::aabb(const std::vector<math::float3>& vertices, const math::float4x4& transform)
{
    math::float4x4_simd world_matrix = math::simd::load(transform);

    float inf = std::numeric_limits<float>::infinity();
    math::float4_simd min = math::simd::load(math::float4_align{inf, inf, inf, 1.0});
    math::float4_simd max = math::simd::load(math::float4_align{-inf, -inf, -inf, 1.0});

    for (auto& vertex : vertices)
    {
        math::float4_simd v = math::simd::load(vertex, 1.0f);
        v = math::matrix_simd::mul(v, world_matrix);
        min = math::simd::min(min, v);
        max = math::simd::max(max, v);
    }

    math::simd::store(min, m_aabb.min);
    math::simd::store(max, m_aabb.max);
}
} // namespace ash::scene