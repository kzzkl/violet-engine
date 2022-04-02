#pragma once

#include "bt3_common.hpp"

namespace ash::physics::bullet3
{
class bt3_shape : public collision_shape_interface
{
public:
    bt3_shape(const collision_shape_desc& desc);

    btCollisionShape* shape() const noexcept { return m_shape.get(); }

private:
    std::unique_ptr<btCollisionShape> m_shape;
};
} // namespace ash::physics::bullet3