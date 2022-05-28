#pragma once

#include "ui/controls/panel.hpp"

namespace ash::editor
{
class hierarchy_view : public ui::panel
{
public:
    hierarchy_view();
    virtual ~hierarchy_view();

    virtual void tick() override;
};
} // namespace ash::editor