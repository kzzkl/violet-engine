#include "component_view.hpp"
#include "transform.hpp"

namespace ash::editor
{
class information_plane : public component_plane
{
public:
    information_plane()
        : component_plane("information", ecs::component_index::value<ecs::information>())
    {
    }

    virtual void draw(ecs::entity entity, ui::ui& ui, ecs::world& world) override
    {
        auto& information = world.component<ecs::information>(entity);
        ui.text(information.name);
    }
};

class transform_plane : public component_plane
{
public:
    transform_plane()
        : component_plane("transform", ecs::component_index::value<scene::transform>())
    {
    }

    virtual void draw(ecs::entity entity, ui::ui& ui, ecs::world& world) override
    {
        auto& transform = world.component<scene::transform>(entity);
    }
};

component_view::component_view(ecs::world& world) : m_world(world), m_planes(ecs::MAX_COMPONENT)
{
    register_plane<ecs::information, information_plane>();
}

void component_view::draw(ui::ui& ui, editor_data& data)
{
    ui.window("component");
    if (data.active_entity != ecs::INVALID_ENTITY)
    {
        for (auto component : m_world.owned_component(data.active_entity))
        {
            auto plane = m_planes[component].get();
            if (plane != nullptr && m_world.has_component(data.active_entity, component))
            {
                if (ui.collapsing(plane->name()))
                    plane->draw(data.active_entity, ui, m_world);
            }
        }
    }
    ui.window_pop();
}
} // namespace ash::editor