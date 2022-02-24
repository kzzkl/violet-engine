#pragma once

#include "ecs_exports.hpp"
#include <bitset>
#include <cstdint>
#include <vector>

namespace ash::ecs
{
using component_index = uint32_t;

static constexpr std::size_t MAX_COMPONENT = 512;
using component_mask = std::bitset<MAX_COMPONENT>;

namespace detail
{
component_index ECS_API next_component_index();
} // namespace detail

template <typename T>
struct component_trait
{
    static component_index index()
    {
        static component_index index = detail::next_component_index();
        return index;
    }
};

template <typename... Components>
class component_list
{
public:
    template <typename... AddTypes>
    using add = component_list<Components..., AddTypes...>;

public:
    template <typename Functor>
    static void each(Functor&& f)
    {
        (f.template operator()<Components>(), ...);
    }

    template <typename Component>
    constexpr static bool has()
    {
        return (std::is_same<Component, Components>::value || ...);
    }

    static component_mask get_mask()
    {
        component_mask result;
        (result.set(component_trait<Components>::index()), ...);
        return result;
    }
};
} // namespace ash::ecs