#pragma once

#include "editor_component.hpp"

namespace ash::editor
{
class component_plane
{
public:
    component_plane(std::string_view name, ecs::component_id component) noexcept
        : m_component(component),
          m_name(name)
    {
    }
    virtual ~component_plane() = default;

    virtual void draw(ecs::entity entity, ui::ui& ui, ecs::world& world) = 0;
    inline ecs::component_id component() const noexcept { return m_component; }
    inline std::string_view name() const noexcept { return m_name; }

private:
    ecs::component_id m_component;
    std::string m_name;
};

class component_view : public editor_view
{
public:
    component_view(ecs::world& world);

    template <typename Component, typename Plane, typename... Args>
    void register_plane(Args&&... args)
    {
        auto component_id = ecs::component_index::value<Component>();
        m_planes[component_id] = std::make_unique<Plane>(std::forward<Args>(args)...);
    }

    virtual void draw(ui::ui& ui, editor_data& data) override;

private:
    ecs::world& m_world;
    std::vector<std::unique_ptr<component_plane>> m_planes;
};
} // namespace ash::editor