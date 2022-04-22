#pragma once

#include "editor_component.hpp"

namespace ash::editor
{
class hierarchy_view : public editor_view
{
public:
    hierarchy_view(ecs::entity scene_root, ecs::world& world);
    virtual ~hierarchy_view() = default;

    virtual void draw(ui::ui& ui, editor_data& data) override;

private:
    void draw_node(ui::ui& ui, ecs::entity entity, editor_data& data);

    ecs::world& m_world;
    ecs::entity m_scene_root;
};
} // namespace ash::editor