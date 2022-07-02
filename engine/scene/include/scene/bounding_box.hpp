#pragma once

#include "math/math.hpp"
#include <vector>

namespace ash::scene
{
struct bounding_volume_aabb
{
    math::float3 min;
    math::float3 max;
};

class bounding_box
{
public:
    bounding_box();

    void aabb(const std::vector<math::float3>& vertices, const math::float4x4& transform);
    const bounding_volume_aabb& aabb() const noexcept { return m_aabb; }

    std::size_t proxy_id() const noexcept { return m_proxy_id; }
    void proxy_id(std::size_t id) noexcept { m_proxy_id = id; }

private:
    bounding_volume_aabb m_aabb;

    std::size_t m_proxy_id;
};
} // namespace ash::scene