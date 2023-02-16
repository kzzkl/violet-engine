#pragma once

#include "common/index_generator.hpp"
#include <bitset>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace violet::ecs
{
using component_id = std::uint16_t;

struct component_index : public index_generator<component_index, component_id>
{
};

class component_constructer
{
public:
    virtual ~component_constructer() = default;

    virtual void construct(void* target) {}
    virtual void move_construct(void* source, void* target) {}
    virtual void destruct(void* target) {}
    virtual void swap(void* a, void* b) {}
};

template <typename Component>
class default_component_constructer : public component_constructer
{
public:
    virtual ~default_component_constructer() = default;

    virtual void construct(void* target) override { new (target) Component(); }
    virtual void move_construct(void* source, void* target) override
    {
        new (target) Component(std::move(*static_cast<Component*>(source)));
    }
    virtual void destruct(void* target) override { static_cast<Component*>(target)->~Component(); }
    virtual void swap(void* a, void* b) override
    {
        std::swap(*static_cast<Component*>(a), *static_cast<Component*>(b));
    }
};

class component_info
{
public:
    component_info() noexcept : m_size(0), m_align(0), m_constructer(nullptr) {}
    component_info(std::size_t size, std::size_t align, component_constructer* constructer) noexcept
        : m_size(size),
          m_align(align),
          m_constructer(constructer)
    {
    }

    void construct(void* target) const { m_constructer->construct(target); }
    void move_construct(void* source, void* target) const
    {
        m_constructer->move_construct(source, target);
    }
    void destruct(void* target) const { m_constructer->destruct(target); }
    void swap(void* a, void* b) const { m_constructer->swap(a, b); }

    std::size_t size() const noexcept { return m_size; }
    std::size_t align() const noexcept { return m_align; }

private:
    std::size_t m_size;
    std::size_t m_align;

    std::unique_ptr<component_constructer> m_constructer;
};

static constexpr std::size_t MAX_COMPONENT = 512;
using component_mask = std::bitset<MAX_COMPONENT>;

using component_registry = std::array<component_info, MAX_COMPONENT>;
} // namespace violet::ecs