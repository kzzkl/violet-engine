#pragma once

#include "ecs/archetype.hpp"
#include "ecs/entity.hpp"

namespace violet
{
class world;
class view_base
{
public:
    view_base(world* world) noexcept;
    virtual ~view_base();

protected:
    const std::vector<archetype*>& get_archetypes(
        const component_mask& include_mask,
        const component_mask& exclude_mask);

    world* get_world() const noexcept
    {
        return m_world;
    }

private:
    world* m_world;
    std::uint32_t m_view_version{0};
    std::vector<archetype*> m_archetypes;
};

template <typename... Components>
struct component_list
{
    template <typename T>
    using append = component_list<Components..., T>;

    using tuple = std::tuple<Components&...>;

    static const component_mask& get_mask()
    {
        static component_mask mask = []()
        {
            component_mask mask;
            (mask.set(component_index::value<Components>()), ...);
            return mask;
        }();
        return mask;
    }

    static std::tuple<Components*...> get_components(
        archetype* archetype,
        std::size_t chunk_index,
        std::uint32_t world_version)
    {
        return archetype->get_components<Components...>(
            chunk_index * archetype->get_chunk_capacity(),
            world_version);
    }

    static bool is_updated(
        archetype* archetype,
        std::size_t chunk_index,
        std::uint32_t system_version)
    {
        return archetype->is_updated<Components...>(chunk_index, system_version);
    }
};

template <typename Functor, typename T>
concept view_callback = requires(Functor functor, T& tuple) {
    { std::apply(functor, tuple) } -> std::same_as<void>;
};

template <
    typename parameter_list = component_list<>,
    typename include_list = component_list<>,
    typename exclude_list = component_list<>>
class view : public view_base
{
public:
    view(world* world)
        : view_base(world)
    {
    }

    template <typename T>
    auto read()
    {
        using new_parameter_list = typename parameter_list::template append<const T>;
        using new_include_list = typename include_list::template append<T>;
        using result_view = view<new_parameter_list, new_include_list, exclude_list>;
        return result_view(get_world());
    }

    template <typename T>
    auto write()
        requires(!std::is_same_v<T, entity>)
    {
        using new_parameter_list = typename parameter_list::template append<T>;
        using new_include_list = typename include_list::template append<T>;
        using result_view = view<new_parameter_list, new_include_list, exclude_list>;
        return result_view(get_world());
    }

    template <typename T>
    auto with()
    {
        using new_include_list = typename include_list::template append<T>;
        using result_view = view<parameter_list, new_include_list, exclude_list>;
        return result_view(get_world());
    }

    template <typename T>
    auto without()
    {
        using new_exclude_list = typename exclude_list::template append<T>;
        using result_view = view<parameter_list, include_list, new_exclude_list>;
        return result_view(get_world());
    }

    template <typename Functor>
        requires view_callback<Functor, typename parameter_list::tuple>
    void each(Functor functor)
    {
        for (auto archetype : get_archetypes(include_list::get_mask(), exclude_list::get_mask()))
        {
            std::size_t chunk_count = archetype->get_chunk_count();
            for (std::size_t i = 0; i < chunk_count; ++i)
            {
                auto components =
                    parameter_list::get_components(archetype, i, get_world()->get_version());

                std::size_t entity_count = archetype->get_entity_count(i);
                for (std::size_t j = 0; j < entity_count; ++j)
                {
                    std::apply(
                        [&](auto&... args)
                        {
                            functor(*(args + j)...);
                        },
                        components);
                }
            }
        }
    }

    template <typename Functor, typename Filter>
        requires view_callback<Functor, typename parameter_list::tuple>
    void each(Functor functor, Filter filter)
    {
        for (auto archetype : get_archetypes(include_list::get_mask(), exclude_list::get_mask()))
        {
            m_archetype = archetype;

            std::size_t chunk_count = archetype->get_chunk_count();
            for (std::size_t i = 0; i < chunk_count; ++i)
            {
                m_chunk_index = i;

                if (!filter(*this))
                {
                    continue;
                }

                auto components =
                    parameter_list::get_components(archetype, i, get_world()->get_version());

                std::size_t entity_count = archetype->get_entity_count(i);
                for (std::size_t j = 0; j < entity_count; ++j)
                {
                    std::apply(
                        [&](auto&... args)
                        {
                            functor(*(args + j)...);
                        },
                        components);
                }
            }
        }
    }

    template <typename T>
    [[nodiscard]] bool is_updated(std::uint32_t system_version) const
    {
        return m_archetype->is_updated<T>(m_chunk_index, system_version);
    }

private:
    archetype* m_archetype{nullptr};
    std::size_t m_chunk_index{0};
};
} // namespace violet