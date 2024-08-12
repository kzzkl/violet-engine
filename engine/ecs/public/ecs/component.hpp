#pragma once

#include "common/type_index.hpp"
#include <bitset>
#include <cstdint>
#include <memory>
#include <array>

namespace violet
{
using component_id = std::uint16_t;

struct component_index : public type_index<component_index, component_id>
{
};

class component_constructor_base
{
public:
    component_constructor_base(std::size_t size, std::size_t align, component_id id) noexcept
        : m_size(size),
          m_align(align),
          m_id(id)
    {
    }
    virtual ~component_constructor_base() = default;

    virtual void construct(void* target) = 0;
    virtual void move_construct(void* source, void* target) = 0;
    virtual void destruct(void* target) = 0;
    virtual void move_assign(void* source, void* target) = 0;

    std::size_t get_size() const noexcept
    {
        return m_size;
    }
    std::size_t get_align() const noexcept
    {
        return m_align;
    }

    component_id get_id() const noexcept
    {
        return m_id;
    }

private:
    std::size_t m_size;
    std::size_t m_align;

    component_id m_id;
};

template <typename Component>
class component_constructor : public component_constructor_base
{
public:
    component_constructor()
        : component_constructor_base(
              sizeof(Component), alignof(Component), component_index::value<Component>())
    {
    }

    void construct(void* target) override
    {
        if constexpr (std::is_constructible_v<Component>)
            new (target) Component();
        else
            throw std::exception("");
    }

    void move_construct(void* source, void* target) override
    {
        new (target) Component(std::move(*static_cast<Component*>(source)));
    }

    void destruct(void* target) override
    {
        static_cast<Component*>(target)->~Component();
    }

    void move_assign(void* source, void* target) override
    {
        *static_cast<Component*>(target) = std::move(*static_cast<Component*>(source));
    }
};

static constexpr std::size_t MAX_COMPONENT = 512;
using component_mask = std::bitset<MAX_COMPONENT>;

using component_table = std::array<std::unique_ptr<component_constructor_base>, MAX_COMPONENT>;
} // namespace violet