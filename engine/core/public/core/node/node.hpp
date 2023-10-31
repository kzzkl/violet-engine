#pragma once

#include "core/node/world.hpp"
#include <string_view>

namespace violet
{
class node
{
public:
    template <typename T>
    class component_handle
    {
    public:
        component_handle(node* node = nullptr) : m_version(-1), m_node(node), m_pointer(nullptr) {}

        node* get_node() const noexcept { return m_node; }

        T* operator->() const
        {
            sync_pointer();
            return m_pointer;
        }

        T& operator*() const
        {
            sync_pointer();
            return *m_pointer;
        }

        operator bool() const noexcept { return m_node != nullptr; }

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
    node(std::string_view name, world& world) noexcept;
    node(const node&) = delete;
    ~node();

    void add_child(node* child);
    void remove_child(node* child);

    template <typename Component>
    [[nodiscard]] component_handle<Component> get_component()
    {
        return component_handle<Component>(this);
    }

    template <typename... Components>
    auto add_component()
    {
        m_world.add_component<Components...>(m_entity);
        return std::make_tuple(get_component<Components>()...);
    }

    template <typename... Components>
    void remove_component()
    {
        m_world.remove_component<Components...>(m_entity);
    }

    template <typename Component>
    [[nodiscard]] bool has_component()
    {
        return m_world.has_component<Component>(m_entity);
    }

    [[nodiscard]] world& get_world() const { return m_world; }

    node& operator=(const node&) = delete;

private:
    std::string m_name;
    entity m_entity;
    world& m_world;
};

template <typename T>
using component_ptr = node::component_handle<T>;
} // namespace violet