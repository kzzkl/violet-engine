#include "scene/bounding_box.hpp"
#include <limits>

namespace ash::scene
{
bounding_box::bounding_box()
    : m_aabb{},
      m_internal_aabb{},
      m_mesh_aabb{},
      m_fatten(0.0f),
      m_proxy_id(-1),
      m_visible(false),
      m_dynamic(false)
{
}

bool bounding_box::transform(const math::float4x4& transform)
{
    math::float4_simd axis =
        math::simd::set(transform[0][0], transform[1][0], transform[2][0], 0.0f);
    math::float4_simd xa = math::vector_simd::mul(axis, math::simd::set(m_mesh_aabb.min[0]));
    math::float4_simd xb = math::vector_simd::mul(axis, math::simd::set(m_mesh_aabb.max[0]));

    axis = math::simd::set(transform[0][1], transform[1][1], transform[2][1], 0.0f);
    math::float4_simd ya = math::vector_simd::mul(axis, math::simd::set(m_mesh_aabb.min[1]));
    math::float4_simd yb = math::vector_simd::mul(axis, math::simd::set(m_mesh_aabb.max[1]));

    axis = math::simd::set(transform[0][2], transform[1][2], transform[2][2], 0.0f);
    math::float4_simd za = math::vector_simd::mul(axis, math::simd::set(m_mesh_aabb.min[2]));
    math::float4_simd zb = math::vector_simd::mul(axis, math::simd::set(m_mesh_aabb.max[2]));

    math::float4_simd min = math::simd::load(transform[3]);
    math::float4_simd max = min;

    min = math::vector_simd::add(min, math::simd::min(xa, xb));
    min = math::vector_simd::add(min, math::simd::min(ya, yb));
    min = math::vector_simd::add(min, math::simd::min(za, zb));

    max = math::vector_simd::add(max, math::simd::max(xa, xb));
    max = math::vector_simd::add(max, math::simd::max(ya, yb));
    max = math::vector_simd::add(max, math::simd::max(za, zb));

    math::simd::store(min, m_internal_aabb.min);
    math::simd::store(max, m_internal_aabb.max);

    bool inside =
        (m_internal_aabb.min[0] >= m_aabb.min[0] && m_internal_aabb.max[0] <= m_aabb.max[0]) &&
        (m_internal_aabb.min[1] >= m_aabb.min[1] && m_internal_aabb.max[1] <= m_aabb.max[1]) &&
        (m_internal_aabb.min[2] >= m_aabb.min[2] && m_internal_aabb.max[2] <= m_aabb.max[2]);

    if (!inside)
    {
        math::float4_simd f = math::simd::set(m_fatten);
        min = math::vector_simd::sub(min, f);
        max = math::vector_simd::add(max, f);
        math::simd::store(min, m_aabb.min);
        math::simd::store(max, m_aabb.max);
        return true;
    }
    else
    {
        return false;
    }
}

void bounding_box::aabb(
    const std::vector<math::float3>& vertices,
    const math::float4x4& transform,
    bool dynamic,
    float fatten)
{
    math::float4x4_simd world_matrix = math::simd::load(transform);

    float inf = std::numeric_limits<float>::infinity();
    math::float4_simd min = math::simd::set(inf, inf, inf, 1.0);
    math::float4_simd max = math::simd::set(-inf, -inf, -inf, 1.0);

    if (dynamic)
    {
        for (auto& vertex : vertices)
        {
            math::float4_simd v = math::simd::load(vertex, 1.0f);
            min = math::simd::min(min, v);
            max = math::simd::max(max, v);
        }

        math::simd::store(min, m_mesh_aabb.min);
        math::simd::store(max, m_mesh_aabb.max);

        m_fatten = fatten;
        this->transform(transform);
    }
    else
    {
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
}
} // namespace ash::scene