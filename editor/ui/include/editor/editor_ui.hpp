#pragma once

#include "editor/hierarchy_view.hpp"
#include "editor/scene_view.hpp"
#include <memory>

namespace ash::editor
{
class editor_ui
{
public:
    editor_ui();

    void tick();

private:
    ecs::entity m_entity;
    ecs::entity m_left_container;
    ecs::entity m_right_container;

    std::unique_ptr<scene_view> m_scene_view;
    std::unique_ptr<hierarchy_view> m_hierarchy_view;
};
} // namespace ash::editor