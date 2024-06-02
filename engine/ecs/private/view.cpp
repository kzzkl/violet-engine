#include "ecs/view.hpp"
#include "ecs/world.hpp"

namespace violet
{
view_base::view_base(world& world) noexcept : m_world(&world)
{
}

view_base::~view_base()
{
}

void view_base::set_mask(const component_mask& mask) noexcept
{
    m_mask = mask;
    m_version = -1;
}

const std::vector<archetype*>& view_base::sync_archetype_list()
{
    if (m_version != m_world->m_view_version)
    {
        m_archetypes.clear();
        for (auto& [mask, archetype] : m_world->m_archetypes)
        {
            if ((m_mask & archetype->get_mask()) == m_mask)
                m_archetypes.push_back(archetype.get());
        }

        m_version = m_world->m_view_version;
    }

    return m_archetypes;
}
} // namespace violet