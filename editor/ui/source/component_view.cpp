#include "component_view.hpp"
#include "transform.hpp"
#include "ui.hpp"
#include "world.hpp"

namespace ash::editor
{
class information_plane : public component_plane
{
public:
    information_plane(core::context* context)
        : component_plane("Information", ecs::component_index::value<ecs::information>(), context)
    {
    }

    virtual void draw(ecs::entity entity) override
    {
        auto& ui = system<ui::ui>();
        auto& world = system<ecs::world>();

        auto& information = world.component<ecs::information>(entity);
        ui.text(information.name);
    }
};

class transform_plane : public component_plane
{
public:
    transform_plane(core::context* context)
        : component_plane("Transform", ecs::component_index::value<scene::transform>(), context)
    {
    }

    virtual void draw(ecs::entity entity) override
    {
        auto& ui = system<ui::ui>();
        auto& transform = system<ecs::world>().component<scene::transform>(entity);

        if (ui.drag("  Position", transform.position))
            transform.dirty = true;

        math::float3 euler = math::euler_plain::rotation_quaternion(transform.rotation);
        if (ui.drag("  Rotation", euler))
        {
            transform.rotation = math::quaternion_plain::rotation_euler(euler);
            transform.dirty = true;
        }

        if (ui.drag("  Scale", transform.scaling))
            transform.dirty = true;
    }
};

class visual_plane : public component_plane
{
public:
    visual_plane(core::context* context)
        : component_plane("Visual", ecs::component_index::value<graphics::visual>(), context)
    {
    }

    virtual void draw(ecs::entity entity) override
    {
        auto& ui = system<ui::ui>();
        auto& visual = system<ecs::world>().component<graphics::visual>(entity);
    }
};

component_view::component_view(core::context* context)
    : editor_view(context),
      m_planes(ecs::MAX_COMPONENT)
{
    register_plane<ecs::information, information_plane>();
    register_plane<scene::transform, transform_plane>();
    register_plane<graphics::visual, visual_plane>();
}

void component_view::draw(editor_data& data)
{
    auto& ui = system<ui::ui>();
    auto& world = system<ecs::world>();

    ui.window("Component");
    if (data.active_entity != ecs::INVALID_ENTITY)
    {
        for (auto component : world.owned_component(data.active_entity))
        {
            auto plane = m_planes[component].get();
            if (plane != nullptr && world.has_component(data.active_entity, component))
            {
                if (ui.collapsing(plane->name()))
                    plane->draw(data.active_entity);
            }
        }
    }
    ui.window_pop();
}
} // namespace ash::editor