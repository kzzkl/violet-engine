#include "world.hpp"

namespace ash::ecs
{
world::world() noexcept
{
}

archetype* world::create_archetype(const archetype_layout& layout)
{
    return (m_archetypes[layout.get_mask()] = std::make_unique<archetype>(layout)).get();
}
} // namespace ash::ecs