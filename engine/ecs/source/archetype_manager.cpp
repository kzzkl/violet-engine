#include "archetype_manager.hpp"

namespace ash::ecs
{
archetype* archetype_manager::create_archetype(const archetype_layout& layout)
{
    return (m_archetypes[layout.get_mask()] = std::make_unique<archetype>(layout)).get();
}
} // namespace ash::ecs