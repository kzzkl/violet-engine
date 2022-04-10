#pragma once

#include "entity_record.hpp"

namespace ash::ecs
{
template <typename Component>
class component_handle_strong
{
public:
    using component_type = Component;
    using entity_type = entity;

public:
    component_handle_strong() noexcept
        : m_component(nullptr),
          m_entity{INVALID_ENTITY_ID, 0},
          m_record(nullptr)
    {
    }

    component_handle_strong(entity entity, entity_record* record) noexcept
        : m_component(nullptr),
          m_entity(entity),
          m_record(record)
    {
        update();
    }

    entity_type entity() const noexcept { return m_entity; }

    component_type& operator*() const { return *get(); }
    component_type* operator->() const { return get(); }

protected:
    component_type* get() const
    {
        if (!m_record->vaild(m_entity))
            update();
        return m_component;
    }

private:
    void update() const
    {
        auto& record = (*m_record)[m_entity];
        m_entity.version = record.version;

        auto handle = record.archetype->begin<component_type>() + record.index;
        m_component = &handle.component<component_type>();
    }

    mutable component_type* m_component;

    mutable entity_type m_entity;
    entity_record* m_record;
};

template <typename Component>
class read_handle_strong : public component_handle_strong<Component>
{
public:
    using component_type = Component;
    using entity_type = entity;
    using base_type = component_handle_strong<Component>;

public:
    read_handle_strong() noexcept : base_type() {}
    read_handle_strong(entity entity, entity_record* record) noexcept : base_type(entity, record) {}

    const component_type& operator*() const { return *get(); }
    const component_type* operator->() const { return get(); }

protected:
    const component_type* get() const
    {
        component_type* result = base_type::get();

        static constexpr bool has_mark = requires(component_type t) { t.mark_read(); };
        if constexpr (has_mark) result->mark_read();

        return result;
    }
};
template <typename Component>
using read = read_handle_strong<Component>;

template <typename Component>
class write_handle_strong : public read_handle_strong<Component>
{
public:
    using component_type = Component;
    using entity_type = entity;
    using base_type = read_handle_strong<Component>;

public:
    write_handle_strong() noexcept : base_type() {}
    write_handle_strong(entity entity, entity_record* record) noexcept : base_type(entity, record)
    {
    }

    component_type& operator*() const { return *get(); }
    component_type* operator->() const { return get(); }

protected:
    component_type* get() const
    {
        component_type* result = const_cast<component_type*>(base_type::get());

        static constexpr bool has_mark = requires(component_type t) { t.mark_write(); };
        if constexpr (has_mark) result->mark_write();

        return result;
    }
};
template <typename Component>
using write = write_handle_strong<Component>;

template <typename Component>
class component_handle_weak
{
public:
    using component_type = Component;
    using entity_type = entity;

public:
    component_handle_weak() noexcept : m_component(nullptr), m_entity{INVALID_ENTITY_ID, 0} {}
    component_handle_weak(entity_type entity, component_type* component)
        : m_entity(entity),
          m_component(component)
    {
    }

    entity_type entity() const noexcept { return m_entity; }

    component_type& operator*() const { return *get(); }
    component_type* operator->() const noexcept { return get(); }

protected:
    component_type* get() const noexcept { return m_component; }

private:
    entity_type m_entity;
    component_type* m_component;
};

template <typename Component>
class read_handle_weak : public component_handle_weak<Component>
{
public:
    using component_type = Component;
    using entity_type = entity;
    using base_type = component_handle_weak<Component>;

public:
    read_handle_weak() noexcept {}
    read_handle_weak(entity_type entity, component_type* component) : base_type(entity, component)
    {
        static constexpr bool has_mark = requires(component_type t) { t.mark_read(); };
        if constexpr (has_mark) component->mark_read();
    }

    const component_type& operator*() const { return *get(); }
    const component_type* operator->() const noexcept { return get(); }

protected:
    const component_type* get() const noexcept { return base_type::get(); }
};
template <typename Component>
using read_weak = read_handle_weak<Component>;

template <typename Component>
class write_handle_weak : public read_handle_weak<Component>
{
public:
    using component_type = Component;
    using entity_type = entity;
    using base_type = read_handle_weak<Component>;

public:
    write_handle_weak() noexcept {}
    write_handle_weak(entity_type entity, component_type* component) : base_type(entity, component)
    {
        static constexpr bool has_mark = requires(component_type t) { t.mark_write(); };
        if constexpr (has_mark) component->mark_write();
    }

    component_type& operator*() const { return *get(); }
    component_type* operator->() const noexcept { return get(); }

protected:
    component_type* get() const noexcept { return const_cast<component_type*>(base_type::get()); }
};
template <typename Component>
using write_weak = write_handle_weak<Component>;
} // namespace ash::ecs