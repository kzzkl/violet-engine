#pragma once

#include "uuid.hpp"
#include <bitset>
#include <cstdint>
#include <functional>
#include <vector>

namespace ash::ecs
{
using component_id = std::size_t;

template <typename T>
struct component_trait;

template <typename T>
static constexpr component_id component_id_v = component_trait<T>::id;

template <typename... Components>
class component_list
{
public:
    template <typename... AddTypes>
    using add = component_list<Components..., AddTypes...>;

private:
    template <typename... Types>
    struct template_index;

    template <typename T, typename... Types>
    struct template_index<T, T, Types...>
    {
        static constexpr std::size_t value = 0;
    };

    template <typename T, typename U, typename... Types>
    struct template_index<T, U, Types...>
    {
        static_assert(sizeof...(Types) != 0);
        static constexpr std::size_t value = 1 + template_index<T, Types...>::value;
    };

    template <typename T, typename... Types>
    static constexpr std::size_t template_index_v = template_index<T, Types...>::value;

public:
    template <typename Functor>
    static void each(Functor&& f)
    {
        (f.template operator()<Components>(), ...);
    }

    template <typename Component>
    static constexpr std::size_t index()
    {
        return template_index_v<Component, Components...>;
    }

    template <typename Component>
    constexpr static bool has()
    {
        return (std::is_same<Component, Components>::value || ...);
    }
};

struct component_info
{
    std::size_t size;
    std::size_t align;

    std::function<void(void*)> construct;
    std::function<void(void*, void*)> move_construct;
    std::function<void(void*)> destruct;
    std::function<void(void*, void*)> swap;
};

using component_set = std::vector<std::pair<component_id, component_info*>>;

using component_index = std::size_t;

static constexpr std::size_t MAX_COMPONENT = 512;
using component_mask = std::bitset<MAX_COMPONENT>;
} // namespace ash::ecs