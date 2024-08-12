#include "ecs/view.hpp"
#include "ecs/world.hpp"

namespace violet
{
view_base::view_base(world* world) noexcept
    : m_world(world)
{
}

view_base::~view_base() {}

const std::vector<archetype*>& view_base::get_archetypes(
    const component_mask& include_mask, const component_mask& exclude_mask)
{
    if (m_view_version != m_world->get_view_version())
    {
        m_archetypes = m_world->get_archetypes(include_mask, exclude_mask);
        m_view_version = m_world->get_view_version();
    }

    return m_archetypes;
}
} // namespace violet