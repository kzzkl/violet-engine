#pragma once

#include "core/node/archetype.hpp"

namespace violet::core
{
template <typename... Components>
class view
{
public:
    view() { (m_mask.set(component_index::value<Components>()), ...); }

    template <typename Functor>
    void each(archetype& archetype, Functor&& functor)
    {
        if ((m_mask & archetype.mask()) != m_mask)
            return;

        for (std::size_t i = 0; i < archetype.size(); i += archetype.entity_per_chunk())
        {
            auto iter = archetype.begin() + i;
            std::tuple<Components*...> components = {&iter.template component<Components>()...};
            std::size_t counter = std::min(archetype.size() - i, archetype.entity_per_chunk());
            while (counter--)
            {
                std::apply(
                    [&](auto&&... args) {
                        functor(*args...);
                        (++args, ...);
                    },
                    components);
                ++iter;
            }
        }
    }

private:
    component_mask m_mask;
};
} // namespace violet::core