#include "scene/bounding_box.hpp"
#include <limits>

namespace violet::scene
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

bool bounding_box::transform(const float4x4& transform)
{
    float4_simd axis =
        simd::set(transform[0][0], transform[1][0], transform[2][0], 0.0f);
    float4_simd xa = vector_simd::mul(axis, simd::set(m_mesh_aabb.min[0]));
    float4_simd xb = vector_simd::mul(axis, simd::set(m_mesh_aabb.max[0]));

    axis = simd::set(transform[0][1], transform[1][1], transform[2][1], 0.0f);
    float4_simd ya = vector_simd::mul(axis, simd::set(m_mesh_aabb.min[1]));
    float4_simd yb = vector_simd::mul(axis, simd::set(m_mesh_aabb.max[1]));

    axis = simd::set(transform[0][2], transform[1][2], transform[2][2], 0.0f);
    float4_simd za = vector_simd::mul(axis, simd::set(m_mesh_aabb.min[2]));
    float4_simd zb = vector_simd::mul(axis, simd::set(m_mesh_aabb.max[2]));

    float4_simd min = simd::load(transform[3]);
    float4_simd max = min;

    min = vector_simd::add(min, simd::min(xa, xb));
    min = vector_simd::add(min, simd::min(ya, yb));
    min = vector_simd::add(min, simd::min(za, zb));

    max = vector_simd::add(max, simd::max(xa, xb));
    max = vector_simd::add(max, simd::max(ya, yb));
    max = vector_simd::add(max, simd::max(za, zb));

    simd::store(min, m_internal_aabb.min);
    simd::store(max, m_internal_aabb.max);

    bool inside =
        (m_internal_aabb.min[0] >= m_aabb.min[0] && m_internal_aabb.max[0] <= m_aabb.max[0]) &&
        (m_internal_aabb.min[1] >= m_aabb.min[1] && m_internal_aabb.max[1] <= m_aabb.max[1]) &&
        (m_internal_aabb.min[2] >= m_aabb.min[2] && m_internal_aabb.max[2] <= m_aabb.max[2]);

    if (!inside)
    {
        float4_simd f = simd::set(m_fatten);
        min = vector_simd::sub(min, f);
        max = vector_simd::add(max, f);
        simd::store(min, m_aabb.min);
        simd::store(max, m_aabb.max);
        return true;
    }
    else
    {
        return false;
    }
}

void bounding_box::aabb(
    const std::vector<float3>& vertices,
    const float4x4& transform,
    bool dynamic,
    float fatten)
{
    float float_min = std::numeric_limits<float>::lowest();
    float float_max = std::numeric_limits<float>::max();
    float4_simd min = simd::set(float_max, float_max, float_max, 1.0);
    float4_simd max = simd::set(float_min, float_min, float_min, 1.0);

    if (dynamic)
    {
        for (auto& vertex : vertices)
        {
            float4_simd v = simd::load(vertex, 1.0f);
            min = simd::min(min, v);
            max = simd::max(max, v);
        }

        simd::store(min, m_mesh_aabb.min);
        simd::store(max, m_mesh_aabb.max);

        m_fatten = fatten;
        this->transform(transform);
    }
    else
    {
        float4x4_simd world_matrix = simd::load(transform);

        for (auto& vertex : vertices)
        {
            float4_simd v = simd::load(vertex, 1.0f);
            v = matrix_simd::mul(v, world_matrix);
            min = simd::min(min, v);
            max = simd::max(max, v);
        }

        simd::store(min, m_aabb.min);
        simd::store(max, m_aabb.max);
    }

    m_dynamic = dynamic;
}
} // namespace violet::scene