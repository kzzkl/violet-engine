#include "core/node/node.hpp"

namespace violet
{
node::node(std::string_view name, world& world) noexcept
    : m_name(name),
      m_world(world)
{
    m_entity = m_world.create(this);
}

node::~node()
{
    if (m_entity.index != INVALID_ENTITY_INDEX)
        m_world.release(m_entity);
}
} // namespace violet