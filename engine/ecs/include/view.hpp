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

template <typename Functor, typename... Args>
concept view_functor = requires(Functor&& f, Args&... args)
{
    f(args...);
};

template <typename Functor, typename... Args>
concept view_functor_ex = requires(Functor&& f, entity e, Args&... args)
{
    f(e, args...);
};

template <typename... Components>
class view : public view_base
{
public:
    using archetype_type = archetype_wrapper;
    using handle = archetype_type::handle<Components...>;

public:
    view(const component_mask& mask) noexcept : view_base(mask) {}

    template <typename Functor>
    void each(Functor&& functor) requires view_functor<Functor, Components...>
    {
        for (auto& [iter, archetype] : m_list)
        {
            for (std::size_t i = 0; i < archetype->size(); ++i)
            {
                iter.index(i);

                if constexpr (view_functor<Functor, Components...>)
                    functor(iter.template component<Components>()...);
                else
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