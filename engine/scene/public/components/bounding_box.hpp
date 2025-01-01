#pragma once

#include "math/math.hpp"
#include <span>

namespace violet
{
struct bounding_volume_aabb
{
    float3 min;
    float3 max;
};

class bounding_box
{
public:
    bounding_box();

    bool transform(const float4x4& transform);

    void aabb(
        std::span<const float3> vertices,
        const float4x4& transform,
        bool dynamic = false,
        float fatten = 1.0f);
    const bounding_volume_aabb& aabb() const noexcept { return m_aabb; }

    void visible(bool visible) noexcept { m_visible = visible; }
    bool visible() const noexcept { return m_visible; }

    std::size_t proxy_id() const noexcept { return m_proxy_id; }
    void proxy_id(std::size_t id) noexcept { m_proxy_id = id; }

    bool dynamic() const noexcept { return m_dynamic; }

private:
    bounding_volume_aabb m_aabb;
    bounding_volume_aabb m_internal_aabb;
    bounding_volume_aabb m_mesh_aabb;

    float m_fatten;

    std::size_t m_proxy_id;
    bool m_visible;
    bool m_dynamic;
};
} // namespace violet