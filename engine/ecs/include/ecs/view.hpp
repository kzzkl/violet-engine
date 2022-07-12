#pragma once

#include "ecs/archetype.hpp"

namespace ash::ecs
{
class view_base
{
public:
    view_base(const component_mask& mask) noexcept : m_mask(mask) {}
    virtual ~view_base() = default;

    const component_mask& mask() const noexcept { return m_mask; }

    void add_archetype(archetype* archetype) { m_list.push_back(archetype); }

protected:
    template <typename Functor>
    void each_archetype(Functor&& functor)
    {
        for (auto archetype : m_list)
            functor(*archetype);
    }

private:
    std::vector<archetype*> m_list;
    component_mask m_mask;
};

template <typename Functor, typename... Args>
concept view_functor_component = requires(Functor&& f, Args&... args)
{
    f(args...);
};

template <typename Functor, typename... Args>
concept view_functor_entity_component = requires(Functor&& f, entity e, Args&... args)
{
    f(e, args...);
};

template <typename Functor, typename... Args>
concept view_functor =
    view_functor_component<Functor, Args...> || view_functor_entity_component<Functor, Args...>;

template <typename... Components>
class view : public view_base
{
public:
    view(const component_mask& mask) noexcept : view_base(mask) {}

    template <typename Functor>
    void each(Functor&& functor) requires view_functor<Functor, Components...>
    {
        auto each_functor = [&](archetype& archetype) {
            for (std::size_t i = 0; i < archetype.size(); i += archetype.entity_per_chunk())
            {
                auto iter = archetype.begin() + i;
                std::tuple<Components*...> components = {&iter.template component<Components>()...};
                std::size_t counter = std::min(archetype.size() - i, archetype.entity_per_chunk());
                while (counter--)
                {
                    if constexpr (view_functor_component<Functor, Components...>)
                    {
                        std::apply(
                            [&](auto&&... args) {
                                functor(*args...);
                                (++args, ...);
                            },
                            components);
                    }
                    else
                    {
                        std::apply(
                            [&](auto&&... args) {
                                functor(iter.entity(), *args...);
                                (++args, ...);
                            },
                            components);
                        ++iter;
                    }
                }
            }
        };

        each_archetype(each_functor);
    }
};
} // namespace ash::ecs