#pragma once

#include "core/node/world.hpp"
#include <string_view>

namespace violet::core
{

class node
{
public:
    template <typename T>
    class component_handle
    {
    public:
        component_handle(node* node) : m_version(-1), m_node(node), m_pointer(nullptr) {}

        bool is_valid() const
        {
            auto [entity_version, component_version] =
                m_node->get_world().get_version(m_node->m_entity);
            return m_node->m_entity.entity_version == entity_version &&
                   m_version == component_version;
        }

        T* operator->() const
        {
            sync_pointer();
            return m_pointer;
        }

        T* operator*() const
        {
            sync_pointer();
            return *m_pointer;
        }

        operator bool() const { return is_valid(); }

    private:
        void sync_pointer() const
        {
            auto [entity_version, component_version] =
                m_node->get_world().get_version(m_node->m_entity);

            if (m_node->m_entity.entity_version != entity_version)
            {
                m_pointer = nullptr;
            }
            else if (m_version != component_version)
            {
                m_pointer = &m_node->get_world().get_component<T>(m_node->m_entity);
                m_version = component_version;
            }
        }

        mutable std::uint16_t m_version;
        mutable T* m_pointer;

        node* m_node;
    };

public:
    node(std::string_view name, world* world = nullptr) noexcept;
    ~node();

    void add(node* child);
    void remove(node* child);

    [[nodiscard]] node* get_parent() const noexcept { return m_parent; }
    [[nodiscard]] const std::vector<node*> get_children() const noexcept { return m_children; }

    template <typename Component>
    component_handle<Component> get_component()
    {
        return component_handle<Component>(this);
    }

    template <typename... Components>
    void add_component()
    {
        m_world->add<Components...>(m_entity);
    }

    template <typename... Components>
    void remove_component()
    {
        m_world->remove<Components...>(m_entity);
    }

    template <typename Component>
    [[nodiscard]] bool has_component()
    {
        return m_world->has_component<Component>(m_entity);
    }

    world& get_world() const { return *m_world; }

private:
    std::string m_name;
    entity m_entity;
    world* m_world;

    node* m_parent;
    std::vector<node*> m_children;
};

template <typename T>
using component_ptr = node::component_handle<T>;
} // namespace violet::core