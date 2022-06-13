#pragma once

#include "ui/controls/dock_window.hpp"

namespace ash::editor
{
class editor_view : public ui::dock_window
{
public:
    editor_view(std::string_view title, std::uint32_t icon_index);

private:
};
} // namespace ash::editor