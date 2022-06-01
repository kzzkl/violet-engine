#include "editor/hierarchy_view.hpp"
#include "core/context.hpp"
#include "core/relation.hpp"
#include "scene/scene.hpp"
#include "scene/scene_event.hpp"
#include "ui/ui.hpp"

namespace ash::editor
{
hierarchy_view::hierarchy_node::hierarchy_node(hierarchy_view* view)
    : m_entity(ecs::INVALID_ENTITY),
      m_view(view)
{
    auto& ui = system<ui::ui>();

    m_title = std::make_unique<ui::panel>();
    m_title->resize(0.0f, 30.0f, true, false);
    m_title->padding(10.0f);
    m_title->on_mouse_press = [this](window::mouse_key key, int x, int y) {
        if (m_container->display())
            collapse();
        else
            expand();

        m_view->m_selected = m_entity;
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
}

void hierarchy_view::hierarchy_node::reset(ecs::entity entity)
{
    auto& world = system<ecs::world>();
    auto& ui = system<ui::ui>();

    auto& info = world.component<ecs::information>(entity);
    m_label->text(info.name, ui.font());

    m_entity = entity;
}

void hierarchy_view::hierarchy_node::expand()
{
    m_container->show();

    auto& world = system<ecs::world>();
    auto& link = world.component<core::link>(m_entity);

    for (auto child : link.children)
    {
        auto child_node = m_view->allocate_node();
        child_node->reset(child);
        child_node->link(m_container.get());
    }
}

void hierarchy_view::hierarchy_node::collapse()
{
    m_container->hide();

    std::vector<hierarchy_node*> child_nodes;
    for (auto child_node : m_container->children())
        child_nodes.push_back(static_cast<hierarchy_node*>(child_node));

    for (auto child_node : child_nodes)
    {
        child_node->unlink();
        m_view->deallocate_node(child_node);
    }
}

hierarchy_view::hierarchy_view() : m_selected(ecs::INVALID_ENTITY)
{
    auto& event = system<core::event>();
    auto& scene = system<scene::scene>();

    flex_direction(ui::LAYOUT_FLEX_DIRECTION_COLUMN);

    auto root_node = allocate_node();
    root_node->reset(scene.root());
    root_node->link(this);
}

hierarchy_view::~hierarchy_view()
{
}

hierarchy_view::hierarchy_node* hierarchy_view::allocate_node()
{
    if (m_free_node.empty())
    {
        m_node_pool.push_back(std::make_unique<hierarchy_node>(this));
        return m_node_pool.back().get();
    }
    else
    {
        auto result = m_free_node.front();
        m_free_node.pop();
        return result;
    }
}

void hierarchy_view::deallocate_node(hierarchy_node* node)
{
    m_free_node.push(node);
}
} // namespace ash::editor