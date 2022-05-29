#include "editor/hierarchy_view.hpp"
#include "core/context.hpp"
#include "core/relation.hpp"
#include "scene/scene.hpp"
#include "scene/scene_event.hpp"
#include "ui/ui.hpp"

namespace ash::editor
{
hierarchy_node::hierarchy_node(ecs::entity entity, ecs::entity* selected)
    : m_entity(entity),
      m_selected(selected)
{
    auto& ui = system<ui::ui>();

    m_title = std::make_unique<ui::panel>();
    m_title->resize(0.0f, 30.0f, true, false);
    m_title->padding(10.0f);
    m_title->on_mouse_click = [this](window::mouse_key key) {
        if (m_container->display())
            m_container->hide();
        else
            m_container->show();

        *m_selected = m_entity;
        return false;
    };
    m_title->link(this);

    m_label = std::make_unique<ui::label>("none", ui.font());
    m_label->resize(100.0f, 100.0f, false, false, true, true);
    m_label->link(m_title.get());

    m_container = std::make_unique<ui::element>();
    m_container->flex_direction(ui::LAYOUT_FLEX_DIRECTION_COLUMN);
    m_container->resize(0.0f, 0.0f, true, true);
    m_container->padding(10.0f);
    m_container->hide();
    m_container->link(this);

    reset(entity);
}

void hierarchy_node::reset(ecs::entity entity)
{
    auto& world = system<ecs::world>();
    auto& ui = system<ui::ui>();

    auto& info = world.component<ecs::information>(entity);
    m_label->text(info.name, ui.font());

    log::debug("{}", info.name);
}

hierarchy_view::hierarchy_view() : ui::panel(ui::COLOR_BLUE_VIOLET), m_selected(ecs::INVALID_ENTITY)
{
    auto& event = system<core::event>();
    auto& scene = system<scene::scene>();

    flex_direction(ui::LAYOUT_FLEX_DIRECTION_COLUMN);

    auto root_node = get_or_create_node(scene.root());
    root_node->link(this);

    event.subscribe<scene::event_enter_scene>("hierarchy view", [this](ecs::entity entity) {
        auto& world = system<ecs::world>();
        auto& link = world.component<core::link>(entity);

        if (link.parent != ecs::INVALID_ENTITY)
        {
            auto parent_node = m_node_pool[link.parent.index].get();
            auto node = get_or_create_node(entity);

            node->link(parent_node->container());
        }
    });

    event.subscribe<scene::event_exit_scene>("hierarchy view", [this](ecs::entity entity) {
        auto node = get_or_create_node(entity);
        node->unlink();;
    });
}

hierarchy_view::~hierarchy_view()
{
}

hierarchy_node* hierarchy_view::get_or_create_node(ecs::entity entity)
{
    if (m_node_pool.size() <= entity.index)
        m_node_pool.resize(entity.index + 1);

    if (m_node_pool[entity.index] == nullptr)
        m_node_pool[entity.index] = std::make_unique<hierarchy_node>(entity, &m_selected);

    return m_node_pool[entity.index].get();
}
} // namespace ash::editor