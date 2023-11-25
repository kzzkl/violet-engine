#pragma once

#include "core/ecs/world.hpp"
#include <string_view>

namespace violet
{
class actor
{
public:
    template <typename T>
    class component_handle
    {
    public:
        component_handle(actor* owner = nullptr) : m_owner(owner) {}

        actor* get_owner() const noexcept { return m_owner; }
        T* get() const noexcept
        {
            return &m_owner->get_world().get_component<T>(m_owner->m_entity);
        }

        T* operator->() const { return get(); }
        T& operator*() const { return m_owner->get_world().get_component<T>(m_owner->m_entity); }

        operator T*() const { return get(); }

        operator bool() const noexcept
        {
            if (m_owner == nullptr)
                return false;

            return m_owner->get_world().is_valid(m_owner->m_entity).first &&
                   m_owner->get_world().has_component<T>(m_owner->m_entity);
        }

    private:
        actor* m_owner;
    };

public:
    actor(std::string_view name, world& world) noexcept;
    actor(const actor&) = delete;
    ~actor();

    template <typename Component>
    [[nodiscard]] component_handle<Component> get()
    {
        return component_handle<Component>(this);
    }

    template <typename... Components>
    auto add()
    {
        m_world.add_component<Components...>(m_entity);
        return std::make_tuple(get<Components>()...);
    }

    template <typename... Components>
    void remove()
    {
        m_world.remove_component<Components...>(m_entity);
    }

    template <typename Component>
    [[nodiscard]] bool has()
    {
        return m_world.has_component<Component>(m_entity);
    }

    [[nodiscard]] const std::string& get_name() const noexcept { return m_name; }
    [[nodiscard]] world& get_world() const { return m_world; }

    actor& operator=(const actor&) = delete;

private:
    std::string m_name;
    entity m_entity;
    world& m_world;
};

template <typename T>
using component_ptr = actor::component_handle<T>;
} // namespace violet