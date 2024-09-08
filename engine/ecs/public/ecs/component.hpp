#pragma once

#include "common/type_index.hpp"
#include <array>
#include <bitset>
#include <cstdint>
#include <memory>

namespace violet
{
using component_id = std::uint16_t;

struct component_index : public type_index<component_index, component_id>
{
};

class component_builder_base
{
public:
    component_builder_base(std::size_t size, std::size_t align, component_id id) noexcept
        : m_size(size),
          m_align(align),
          m_id(id)
    {
    }
    virtual ~component_builder_base() = default;

    virtual void construct(void* target) = 0;
    virtual void copy_construct(const void* source, void* target) = 0;
    virtual void move_construct(void* source, void* target) = 0;
    virtual void destruct(void* target) = 0;
    virtual void move_assignment(void* source, void* target) = 0;

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
class component_builder : public component_builder_base
{
public:
    component_builder()
        : component_builder_base(
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

    void copy_construct(const void* source, void* target) override
    {
        new (target) Component(*static_cast<const Component*>(source));
    }

    void move_construct(void* source, void* target) override
    {
        new (target) Component(std::move(*static_cast<Component*>(source)));
    }

    void destruct(void* target) override
    {
        static_cast<Component*>(target)->~Component();
    }

    void move_assignment(void* source, void* target) override
    {
        *static_cast<Component*>(target) = std::move(*static_cast<Component*>(source));
    }
};

static constexpr std::size_t MAX_COMPONENT = 512;
using component_mask = std::bitset<MAX_COMPONENT>;

using component_builder_list = std::array<std::unique_ptr<component_builder_base>, MAX_COMPONENT>;
} // namespace violet