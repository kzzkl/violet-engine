#pragma once

#include "common/type_index.hpp"
#include <bitset>
#include <cstdint>

namespace violet
{
using component_id = std::uint16_t;

struct component_index : public type_index<component_index, component_id>
{
};

class component_builder
{
public:
    component_builder(std::size_t size, std::size_t align, component_id id) noexcept
        : m_size(size),
          m_align(align),
          m_id(id)
    {
    }
    virtual ~component_builder() = default;

    virtual void construct(void* address) = 0;
    virtual void move_construct(void* src, void* dst) = 0;
    virtual void destruct(void* address) = 0;
    virtual void move_assignment(void* src, void* dst) = 0;

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
class component_builder_default : public component_builder
{
public:
    component_builder_default()
        : component_builder(
              sizeof(Component),
              alignof(Component),
              component_index::value<Component>())
    {
    }

    void construct(void* address) override
    {
        if constexpr (std::is_constructible_v<Component>)
        {
            new (address) Component();
        }
        else
        {
            throw std::exception("");
        }
    }

    void move_construct(void* src, void* dst) override
    {
        new (dst) Component(std::move(*static_cast<Component*>(src)));
    }

    void destruct(void* address) override
    {
        static_cast<Component*>(address)->~Component();
    }

    void move_assignment(void* src, void* dst) override
    {
        *static_cast<Component*>(dst) = std::move(*static_cast<Component*>(src));
    }
};

static constexpr std::size_t MAX_COMPONENT_TYPE = 512;
using component_mask = std::bitset<MAX_COMPONENT_TYPE>;

template <typename Component>
struct component_trait
{
};

template <typename T>
concept is_companion_component = requires { typename component_trait<T>::main_component; };
} // namespace violet