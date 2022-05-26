#include "ui/element_tree.hpp"
#include "core/context.hpp"
#include "core/relation.hpp"
#include "ecs/world.hpp"
#include "ui/controls/container.hpp"
#include "ui/ui_event.hpp"

namespace ash::ui
{
element_tree::element_tree() : m_mesh_pool_index(0), m_window_width(0.0f), m_window_height(0.0f)
{
    auto& world = system<ecs::world>();
    m_view = world.make_view<element>();

    m_root = world.create();
    world.add<core::link, element>(m_root);

    auto& root_element = world.component<element>(m_root);
    root_element.control = std::make_unique<container>();
    root_element.layout.impl()->reset_as_root();
    root_element.layout.flex_direction(LAYOUT_FLEX_DIRECTION_ROW);
    root_element.show = true;

    m_layout = std::make_unique<layout>(root_element.layout);
}

void element_tree::link(element& child, element& parent)
{
    child.layout.parent(&parent.layout);
    child.control->layer(parent.control->layer() + 1);
}

bool element_tree::tick()
{
    auto& event = system<core::event>();
    auto& world = system<ecs::world>();

    auto& root_layout = world.component<element>(m_root).layout;

    bool layout_dirty = false;
    bool control_dirty = false;
    m_view->each([&](element& element) {
        if (!element.show)
            return;

        if (element.layout.dirty())
            layout_dirty = true;

        if (element.control->dirty())
        {
            control_dirty = true;
            element.control->reset_dirty();
        }
    });

    if (!layout_dirty && !control_dirty)
        return false;

    if (layout_dirty)
    {
        log::debug("calculate ui.");
        m_layout->calculate(&root_layout, m_window_width, m_window_height);

        // The node coordinates stored in yoga are the relative coordinates of the parent node,
        // which
        // are converted to absolute coordinates here.
        system<core::relation>().each_bfs(m_root, [&, this](ecs::entity node) {
            if (!world.has_component<element>(node))
                return;

            auto& link = world.component<core::link>(node);
            if (link.parent == ecs::INVALID_ENTITY)
                return;

            auto& parent_element = world.component<element>(link.parent);
            element_extent parent_extent = parent_element.layout.extent();

            auto& node_element = world.component<element>(node);
            node_element.layout.calculate_absolute_position(parent_extent.x, parent_extent.y);
        });

        event.publish<event_calculate_layout>();
    }

    for (auto& mesh : m_mesh_pool)
        mesh->reset();
    m_meshes.clear();
    m_mesh_pool_index = 0;

    m_view->each([this](element& element) {
        if (!element.show || element.control->type() == ELEMENT_CONTROL_TYPE_CONTAINER)
            return;

        element_extent extent = element.layout.extent();
        element.control->extent(extent);

        mesh_key key = {element.control->type(), element.control->mesh().texture};
        if (key.type == ELEMENT_CONTROL_TYPE_IMAGE && key.texture == nullptr)
            return;

        element_mesh* target_mesh = nullptr;
        const element_mesh* source_mesh = &element.control->mesh();

        auto iter = m_meshes.find(key);
        if (iter == m_meshes.end())
        {
            target_mesh = allocate_mesh();
            m_meshes[key] = target_mesh;
        }
        else
        {
            target_mesh = iter->second;
        }

        target_mesh->texture = element.control->mesh().texture;

        std::uint32_t vertex_base = target_mesh->vertex_position.size();
        std::uint32_t index_base = target_mesh->indices.size();
        target_mesh->indices.resize(target_mesh->indices.size() + source_mesh->indices.size());
        std::transform(
            source_mesh->indices.begin(),
            source_mesh->indices.end(),
            target_mesh->indices.begin() + index_base,
            [vertex_base](std::uint32_t index) { return index + vertex_base; });

        target_mesh->vertex_position.insert(
            target_mesh->vertex_position.end(),
            source_mesh->vertex_position.begin(),
            source_mesh->vertex_position.end());
        target_mesh->vertex_uv.insert(
            target_mesh->vertex_uv.end(),
            source_mesh->vertex_uv.begin(),
            source_mesh->vertex_uv.end());
        target_mesh->vertex_color.insert(
            target_mesh->vertex_color.end(),
            source_mesh->vertex_color.begin(),
            source_mesh->vertex_color.end());
    });

    return true;
}

void element_tree::resize(std::uint32_t width, std::uint32_t height)
{
    auto& world = system<ecs::world>();
    auto& root_element = world.component<element>(m_root);
    root_element.layout.resize(width, height);

    m_window_width = static_cast<float>(width);
    m_window_height = static_cast<float>(height);
}

element_mesh* element_tree::allocate_mesh()
{
    if (m_mesh_pool_index >= m_mesh_pool.size())
        m_mesh_pool.push_back(std::make_unique<element_mesh>());

    auto result = m_mesh_pool[m_mesh_pool_index].get();
    ++m_mesh_pool_index;
    return result;
}
} // namespace ash::ui