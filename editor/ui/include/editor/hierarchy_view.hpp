#pragma once

#include "ecs/entity.hpp"

namespace ash::editor
{
class hierarchy_view
{
public:
    hierarchy_view(ecs::entity ui_parent);
    ~hierarchy_view();

private:
    ecs::entity m_ui;
};
} // namespace ash::editor