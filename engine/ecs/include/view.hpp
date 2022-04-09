#pragma once

#include "archetype_wrapper.hpp"
#include <vector>

namespace ash::ecs
{
class view_base
{
public:
    using archetype_type = archetype_wrapper;

public:
    view_base(const component_mask& mask) noexcept : m_mask(mask) {}
    virtual ~view_base() = default;

    const component_mask& mask() const noexcept { return m_mask; }

    virtual void add_archetype(archetype_type* archetype) = 0;

private:
    component_mask m_mask;
};

template <typename... Components>
class view : public view_base
{
public:
    using archetype_type = archetype_wrapper;
    using handle = archetype_type::handle<Components...>;

public:
    view(const component_mask& mask) noexcept : view_base(mask) {}

    void each(std::function<void(Components&...)>&& functor)
    {
        for (auto& [iter, archetype] : m_list)
        {
            for (std::size_t i = 0; i < archetype->size(); ++i)
            {
                iter.index(i);
                functor(iter.template component<Components>()...);
            }

            // Because the handle is to be reused, reset it to the beginning.
            iter.index(0);
        }
    }

    void each(std::function<void(entity, Components&...)>&& functor)
    {
        for (auto& [iter, archetype] : m_list)
        {
            for (std::size_t i = 0; i < archetype->size(); ++i)
            {
                iter.index(i);
                functor(iter.entity(), iter.template component<Components>()...);
            }

            // Because the handle is to be reused, reset it to the beginning.
            iter.index(0);
        }
    }

    virtual void add_archetype(archetype_type* archetype) override
    {
        m_list.push_back(std::make_pair(archetype->begin<Components...>(), archetype));
    }

private:
    std::vector<std::pair<handle, archetype_type*>> m_list;
};
} // namespace ash::ecs