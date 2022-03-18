#pragma once

#include "type_trait.hpp"
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
using component_list = type_list<Components...>;

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

using component_index = std::uint16_t;

static constexpr std::size_t MAX_COMPONENT = 512;
using component_mask = std::bitset<MAX_COMPONENT>;
} // namespace ash::ecs