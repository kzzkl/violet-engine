#include "ecs/actor.hpp"

namespace violet
{
actor::actor(std::string_view name, world& world) noexcept : m_name(name), m_world(world)
{
    m_entity = m_world.create(this);
}

actor::~actor()
{
    if (m_entity.index != INVALID_ENTITY_INDEX)
        m_world.release(m_entity);
}
} // namespace violet