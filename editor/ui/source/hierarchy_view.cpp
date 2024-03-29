#include "editor/hierarchy_view.hpp"
#include "core/context.hpp"
#include "core/relation.hpp"
#include "scene/scene.hpp"
#include "scene/scene_event.hpp"
#include "ui/ui.hpp"

namespace violet::editor
{
hierarchy_view::hierarchy_view(ui::dock_area* area, const ui::dock_window_theme& theme)
    : ui::dock_window("Hierarchy", 0xEEBA, area, theme),
      m_selected(ecs::INVALID_ENTITY)
{
    auto& scene = system<scene::scene>();
    auto& world = system<ecs::world>();

    m_tree = std::make_unique<ui::tree>();
    m_tree->width_percent(100.0f);
    m_tree->on_select = [this](ui::tree_node* node) { m_selected = m_node_to_entity[node]; };
    add_item(m_tree.get());

    load_entity(scene.root());

    auto& event = system<core::event>();
    event.subscribe<scene::event_enter_scene>("hierarchy view", [this](ecs::entity entity) {
        auto& world = system<ecs::world>();
        auto& link = world.component<core::link>(entity);

        auto iter = m_entity_to_node.find(link.parent);
        if (iter == m_entity_to_node.end())
            return;

        if (iter->second.loaded)
            load_entity(entity);
    });
}

hierarchy_view::~hierarchy_view()
{
}

void hierarchy_view::load_entity(ecs::entity entity)
{
    auto& world = system<ecs::world>();
    auto& link = world.component<core::link>(entity);

    ui::tree_node* node = nullptr;
    auto iter = m_entity_to_node.find(entity);
    if (iter == m_entity_to_node.end())
    {
        node = allocate_node(entity, true);
        if (link.parent != ecs::INVALID_ENTITY)
            m_entity_to_node[link.parent].node->add_node(node);
        else
            m_tree->add_node(node);
    }
    else
    {
        if (iter->second.loaded)
            return;

        iter->second.loaded = true;
        node = iter->second.node;
    }

    for (auto child : link.children)
    {
        auto child_node = allocate_node(child, false);
        node->add_node(child_node);
    }
}

void hierarchy_view::unload_entity(ecs::entity entity)
{
    auto& world = system<ecs::world>();
    auto& link = world.component<core::link>(entity);

    ui::tree_node* node = m_entity_to_node[entity].node;
    for (auto child : link.children)
    {
        node->remove(m_entity_to_node[child].node);
        deallocate_node(child);
    }
}

ui::tree_node* hierarchy_view::allocate_node(ecs::entity entity, bool loaded)
{
    ui::tree_node* result = nullptr;

    auto& info = system<ecs::world>().component<ecs::information>(entity);
    if (m_free_node.empty())
    {
        auto node = std::make_unique<ui::tree_node>(
            info.name,
            system<ui::ui>().theme<ui::tree_node_theme>("dark"));
        result = node.get();
        result->on_expand = [entity, result, this]() {
            auto& link = system<ecs::world>().component<core::link>(entity);
            for (auto child : link.children)
                load_entity(child);
        };
        m_node_pool.push_back(std::move(node));
    }
    else
    {
        result = m_free_node.front();
        result->text(info.name);
        m_free_node.pop();
    }

    m_entity_to_node[entity] = {result, loaded};
    m_node_to_entity[result] = entity;
    return result;
}

void hierarchy_view::deallocate_node(ecs::entity entity)
{
    auto node = m_entity_to_node[entity].node;
    m_free_node.push(node);

    m_entity_to_node.erase(entity);
    m_node_to_entity.erase(node);
}
} // namespace violet::editor