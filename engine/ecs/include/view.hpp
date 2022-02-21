#pragma once

#include <forward_list>

namespace ash::ecs
{
template <typename Archetype, typename... Components>
class base_view
{
public:
    template <typename Functor>
    void each(Functor&& functor)
    {
        for (auto archetype : m_list)
        {
            for (auto iter = archetype->begin<Components...>();
                 iter != archetype->end<Components...>();
                 ++iter)
            {
                functor(iter.get_entity(), iter.get_component<Components>()...);
            }
        }
    }

    void insert(Archetype* archetype) { m_list.push_front(archetype); }

private:
    std::forward_list<Archetype*> m_list;
};
} // namespace ash::ecs