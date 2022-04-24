#pragma once

#include "editor_component.hpp"

namespace ash::editor
{
class hierarchy_view : public editor_view
{
public:
    hierarchy_view(core::context* context);
    virtual ~hierarchy_view() = default;

    virtual void draw(editor_data& data) override;

private:
    void draw_node(ecs::entity entity, editor_data& data);
};
} // namespace ash::editor