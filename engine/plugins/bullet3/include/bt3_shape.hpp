#pragma once

#include "bt3_common.hpp"
#include <memory>

namespace violet::bt3
{
class bt3_shape : public phy_collision_shape
{
public:
    bt3_shape(const phy_collision_shape_desc& desc);
    bt3_shape(const phy_collision_shape* const* child, const float4x4* offset, std::size_t size);

    virtual ~bt3_shape() = default;

    btCollisionShape* shape() const noexcept { return m_shape.get(); }

protected:
    std::unique_ptr<btCollisionShape> m_shape;
};
} // namespace violet::bt3