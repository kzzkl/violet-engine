#pragma once

#include "entity_record.hpp"

namespace ash::ecs
{
template <typename Component>
class component_handle
{
public:
    using component_type = Component;
    using entity_type = entity;

public:
    component_handle() : m_pointer(nullptr), m_entity{INVALID_ENTITY_ID, 0}, m_record(nullptr) {}
    component_handle(entity entity, entity_record* record)
        : m_pointer(nullptr),
          m_entity(entity),
          m_record(record)
    {
        update();
    }

    component_type* get() const
    {
        if (!m_record->vaild(m_entity))
            update();
        return m_pointer;
    }

    entity_type entity() const noexcept { return m_entity; }

    component_type& operator*() const { return *get(); }
    component_type* operator->() const { return get(); }

private:
    void update() const
    {
        auto& record = (*m_record)[m_entity];
        m_entity.version = record.version;

        auto handle = record.archetype->begin<component_type>() + record.index;
        m_pointer = &handle.component<component_type>();
    }

    mutable component_type* m_pointer;

    mutable entity_type m_entity;
    entity_record* m_record;
};

template <typename Component>
class read_handle : public component_handle<Component>
{
public:
    using component_type = Component;
    using entity_type = entity;
    using base_type = component_handle<Component>;

public:
    read_handle() : base_type() {}
    read_handle(entity entity, entity_record* record) : base_type(entity, record) {}

    const component_type* get() const
    {
        component_type* result = base_type::get();

        static constexpr bool has_mark = requires(Component t) { t.mark_read(); };
        if constexpr (has_mark) result->mark_read();

        return result;
    }

    const component_type& operator*() const { return *get(); }
    const component_type* operator->() const { return get(); }
};
template <typename Component>
using read = read_handle<Component>;

template <typename Component>
class write_handle : public read_handle<Component>
{
public:
    using component_type = Component;
    using entity_type = entity;
    using base_type = read_handle<Component>;

public:
    write_handle() : base_type() {}
    write_handle(entity entity, entity_record* record) : base_type(entity, record) {}

    component_type* get() const
    {
        component_type* result = const_cast<component_type*>(base_type::get());

        static constexpr bool has_mark = requires(Component t) { t.mark_write(); };
        if constexpr (has_mark) result->mark_write();

        return result;
    }

    component_type& operator*() const { return *get(); }
    component_type* operator->() const { return get(); }
};
template <typename Component>
using write = write_handle<Component>;

template <typename Component>
using read_handle_weak = const Component*;

template <typename Component>
using write_handle_weak = const Component*;
} // namespace ash::ecs