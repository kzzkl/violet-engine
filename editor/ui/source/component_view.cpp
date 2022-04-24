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
        : component_plane("information", ecs::component_index::value<ecs::information>(), context)
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
        : component_plane("transform", ecs::component_index::value<scene::transform>(), context)
    {
    }

    virtual void draw(ecs::entity entity) override
    {
        auto& ui = system<ui::ui>();
        auto& world = system<ecs::world>();
        auto& transform = world.component<scene::transform>(entity);

        ui.text(std::to_string(transform.position[0]));
        ui.text(std::to_string(transform.position[1]));
        ui.text(std::to_string(transform.position[2]));
        ui.text(std::to_string(transform.rotation[0]));
        ui.text(std::to_string(transform.rotation[1]));
        ui.text(std::to_string(transform.rotation[2]));
        ui.text(std::to_string(transform.rotation[3]));
    }
};

component_view::component_view(core::context* context)
    : editor_view(context),
      m_planes(ecs::MAX_COMPONENT)
{
    register_plane<ecs::information, information_plane>();
    register_plane<scene::transform, transform_plane>();
}

void component_view::draw(editor_data& data)
{
    auto& ui = system<ui::ui>();
    auto& world = system<ecs::world>();

    ui.window("component");
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