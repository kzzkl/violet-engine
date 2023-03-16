#pragma once

#include "core/node/archetype.hpp"

namespace violet::core
{
class world;
class view_base
{
public:
    view_base(world& world) noexcept;
    virtual ~view_base();

protected:
    const std::vector<archetype*>& sync_archetype_list();
    void set_mask(const component_mask& mask) noexcept;

    component_mask m_mask;
    std::uint32_t m_version;

    world* m_world;
    std::vector<archetype*> m_archetypes;
};

template <typename... Components>
class view : public view_base
{
public:
    view(world& world) : view_base(world)
    {
        component_mask mask;
        (mask.set(component_index::value<Components>()), ...);
        set_mask(mask);
    }

    template <typename Functor>
    void each(Functor&& functor)
    {
        const std::vector<archetype*>& archetypes = sync_archetype_list();

        for (archetype* archetype : archetypes)
        {
            for (auto iter = archetype->begin(); iter != archetype->end(); ++iter)
            {
                std::tuple<Components*...> components = {
                    &iter.template get_component<Components>()...};
                std::apply(
                    [&](auto&&... args) {
                        functor(*args...);
                        (++args, ...);
                    },
                    components);
            }
        }
    }
};
} // namespace violet::core