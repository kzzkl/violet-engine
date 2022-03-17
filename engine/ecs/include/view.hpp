#pragma once

#include "archetype.hpp"
#include "component.hpp"
#include <vector>

namespace ash::ecs
{
class view_base
{
public:
    view_base(const component_mask& mask) noexcept : m_mask(mask) {}
    virtual ~view_base() = default;

    const component_mask& get_mask() const noexcept { return m_mask; }

    virtual void add_archetype(archetype* archetype) = 0;

private:
    component_mask m_mask;
};

template <typename... Components>
class view : public view_base
{
public:
    using handle = archetype::handle<Components...>;

public:
    view(const component_mask& mask) noexcept : view_base(mask) {}

    template <typename Functor>
    void each(Functor&& functor)
    {
        for (auto& [iter, archetype] : m_list)
        {
            for (std::size_t i = 0; i < archetype->size(); ++i)
            {
                iter.set_index(i);
                functor(iter.template get_component<Components>()...);
            }
            iter.set_index(0);
        }
    }

    virtual void add_archetype(archetype* archetype) override
    {
        m_list.push_back(std::make_pair(archetype->begin<Components...>(), archetype));
    }

private:
    std::vector<std::pair<handle, archetype*>> m_list;
};
} // namespace ash::ecs