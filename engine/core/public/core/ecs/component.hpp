#pragma once

#include "common/type_index.hpp"
#include <bitset>
#include <cstdint>
#include <memory>

namespace violet
{
using component_id = std::uint16_t;

struct component_index : public type_index<component_index, component_id>
{
};

class actor;

class component_info
{
public:
    component_info(std::size_t size, std::size_t align, component_id id) noexcept
        : m_size(size),
          m_align(align),
          m_id(id)
    {
    }
    virtual ~component_info() = default;

    virtual void construct(actor* owner, void* target) = 0;
    virtual void move_construct(actor* owner, void* source, void* target) = 0;
    virtual void destruct(void* target) = 0;
    virtual void swap(void* a, void* b) = 0;

    std::size_t size() const noexcept { return m_size; }
    std::size_t align() const noexcept { return m_align; }

    component_id get_id() const noexcept { return m_id; }

private:
    std::size_t m_size;
    std::size_t m_align;

    component_id m_id;
};

template <typename Component>
class component_info_default : public component_info
{
public:
    component_info_default()
        : component_info(sizeof(Component), alignof(Component), component_index::value<Component>())
    {
    }

    virtual void construct(actor* owner, void* target) override
    {
        if constexpr (std::is_constructible_v<Component>)
            new (target) Component();
        else
            throw std::exception("");
    }
    virtual void move_construct(actor* owner, void* source, void* target) override
    {
        new (target) Component(std::move(*static_cast<Component*>(source)));
    }
    virtual void destruct(void* target) override { static_cast<Component*>(target)->~Component(); }
    virtual void swap(void* a, void* b) override
    {
        std::swap(*static_cast<Component*>(a), *static_cast<Component*>(b));
    }
};

static constexpr std::size_t MAX_COMPONENT = 512;
using component_mask = std::bitset<MAX_COMPONENT>;

using component_table = std::array<std::unique_ptr<component_info>, MAX_COMPONENT>;
} // namespace violet