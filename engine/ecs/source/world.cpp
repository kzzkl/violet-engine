#include "world.hpp"

namespace ash::ecs
{
world::world() : m_nextentity(0)
{
}

entity world::allocate_entity()
{
    entity result;
    if (m_free_entity.empty())
    {
        result = m_nextentity++;
    }
    else
    {
        result = m_free_entity.front();
        m_free_entity.pop();
    }
    return result;
}
} // namespace ash::ecs