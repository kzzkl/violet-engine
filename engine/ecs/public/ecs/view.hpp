#pragma once

#include "ecs/archetype.hpp"

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
        const component_mask& include_mask, const component_mask& exclude_mask);

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
        archetype* archetype, std::size_t chunk_index, std::uint32_t world_version)
    {
        return archetype->get_components<Components...>(chunk_index, 0, world_version);
    }

    static bool is_updated(
        archetype* archetype, std::size_t chunk_index, std::uint32_t system_version)
    {
        return archetype->is_updated<Components...>(chunk_index, system_version);
    }
};

template <typename Functor, typename T>
concept view_callback = requires(Functor functor, T& tuple) {
    { std::apply(functor, tuple) } -> std::same_as<void>;
};

enum view_filter
{
    VIEW_FILTER_NONE = 0,
    VIEW_FILTER_UPDATED = 1 << 0
};
using view_filters = std::uint32_t;

template <typename include_list = component_list<>, typename exclude_list = component_list<>>
class view : public view_base
{
public:
    view(world* world, std::uint32_t system_version, view_filters filters = VIEW_FILTER_NONE)
        : view_base(world),
          m_system_version(system_version),
          m_filters(filters)
    {
    }

    template <typename T>
    auto read()
    {
        using result_view = view<typename include_list::template append<const T>, exclude_list>;
        return result_view(get_world(), m_system_version, m_filters);
    }

    template <typename T>
    auto write()
    {
        using result_view = view<typename include_list::template append<T>, exclude_list>;
        return result_view(get_world(), m_system_version, m_filters);
    }

    template <typename T>
    auto without()
    {
        using result_view = view<include_list, typename exclude_list::template append<T>>;
        return result_view(get_world(), m_system_version, m_filters);
    }

    auto set_filters(view_filters filters) noexcept
    {
        m_filters = filters;
        return *this;
    }

    template <typename Functor>
        requires view_callback<Functor, typename include_list::tuple>
    void each(Functor&& functor)
    {
        for (auto archetype : get_archetypes(include_list::get_mask(), exclude_list::get_mask()))
        {
            std::size_t chunk_count = archetype->get_chunk_count();
            for (std::size_t i = 0; i < chunk_count; ++i)
            {
                if ((m_filters & VIEW_FILTER_UPDATED) &&
                    !include_list::is_updated(archetype, i, m_system_version))
                {
                    continue;
                }

                auto components =
                    include_list::get_components(archetype, i, get_world()->get_version());

                std::size_t entity_count = archetype->get_entity_count(i);
                for (std::size_t j = 0; j < entity_count; ++j)
                {
                    std::apply([&](auto&... args) { functor(*(args + j)...); }, components);
                }
            }
        }
    }

private:
    std::uint32_t m_system_version{0};
    view_filters m_filters{VIEW_FILTER_NONE};
};
} // namespace violet