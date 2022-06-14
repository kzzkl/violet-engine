#pragma once

#include "ui/controls/dock_window.hpp"

namespace ash::editor
{
class editor_view : public ui::dock_window
{
public:
    editor_view(std::string_view title, ui::dock_area* area);

private:
};
} // namespace ash::editor