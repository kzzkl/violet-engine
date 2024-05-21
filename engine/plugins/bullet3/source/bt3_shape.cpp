#include "bt3_shape.hpp"

namespace violet::bt3
{
bt3_shape::bt3_shape(const phy_collision_shape_desc& desc)
{
    switch (desc.type)
    {
    case PHY_COLLISION_SHAPE_TYPE_BOX:
        m_shape = std::make_unique<btBoxShape>(
            btVector3(desc.box.length * 0.5f, desc.box.height * 0.5f, desc.box.width * 0.5f));
        break;
    case PHY_COLLISION_SHAPE_TYPE_SPHERE:
        m_shape = std::make_unique<btSphereShape>(desc.sphere.radius);
        break;
    case PHY_COLLISION_SHAPE_TYPE_CAPSULE:
        if (desc.capsule.height < 0.000001f)
            m_shape = std::make_unique<btSphereShape>(desc.capsule.radius);
        else
            m_shape = std::make_unique<btCapsuleShape>(desc.capsule.radius, desc.capsule.height);
        break;
    default:
        break;
    }
}

bt3_shape::bt3_shape(
    const phy_collision_shape* const* child,
    const float4x4* offset,
    std::size_t size)
{
    auto shape = std::make_unique<btCompoundShape>();

    btTransform local;
    for (std::size_t i = 0; i < size; ++i)
    {
        local.setFromOpenGLMatrix(&offset[i][0][0]);
        shape->addChildShape(local, static_cast<const bt3_shape*>(child[i])->shape());
    }

    m_shape = std::move(shape);
}
} // namespace violet::bt3