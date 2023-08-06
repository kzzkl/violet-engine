#include "core/node/node.hpp"
#include "core/engine.hpp"

namespace violet
{
node::node(std::string_view name, world* world) noexcept
    : m_name(name),
      m_world(world),
      m_parent(nullptr)
{
    if (m_world == nullptr)
        m_world = &engine::get_world();

    m_entity = m_world->create(this);
}

node::~node()
{
    if (m_entity.index != INVALID_ENTITY_INDEX)
        m_world->release(m_entity);
}

void node::add(node* child)
{
    if (child == nullptr || child->m_parent == this)
        return;

    if (child->m_parent != nullptr)
        child->m_parent->remove(child);

    child->m_parent = this;
    m_children.push_back(child);
}

void node::remove(node* child)
{
    for (std::size_t i = 0; i < m_children.size(); ++i)
    {
        if (m_children[i] == child)
        {
            std::swap(m_children[i], m_children.back());
            m_children.pop_back();

            child->m_parent = nullptr;
            return;
        }
    }
}
} // namespace violet