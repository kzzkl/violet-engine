#include "bt3_shape.hpp"

namespace ash::physics::bullet3
{
bt3_shape::bt3_shape(const collision_shape_desc& desc)
{
    switch (desc.type)
    {
    case collision_shape_type::BOX:
        m_shape = std::make_unique<btBoxShape>(
            btVector3(desc.box.length * 0.5f, desc.box.height * 0.5f, desc.box.width * 0.5f));
        break;
    case collision_shape_type::SPHERE:
        m_shape = std::make_unique<btSphereShape>(desc.sphere.radius);
        break;
    case collision_shape_type::CAPSULE:
        if (desc.capsule.height < 0.000001f)
            m_shape = std::make_unique<btSphereShape>(desc.capsule.radius);
        else
            m_shape = std::make_unique<btCapsuleShape>(desc.capsule.radius, desc.capsule.height);
        break;
    default:
        break;
    }
}
} // namespace ash::physics::bullet3