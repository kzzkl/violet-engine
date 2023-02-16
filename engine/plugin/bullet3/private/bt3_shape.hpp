#pragma once

#include "bt3_common.hpp"
#include <vector>

namespace violet::physics::bullet3
{
class bt3_shape : public collision_shape_interface
{
public:
    bt3_shape(const collision_shape_desc& desc);
    bt3_shape(
        const collision_shape_interface* const* child,
        const math::float4x4* offset,
        std::size_t size);

    virtual ~bt3_shape() = default;

    btCollisionShape* shape() const noexcept { return m_shape.get(); }

protected:
    std::unique_ptr<btCollisionShape> m_shape;
};
} // namespace violet::physics::bullet3