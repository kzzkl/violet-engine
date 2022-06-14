#include "editor/editor_view.hpp"
#include "ui/ui.hpp"

namespace ash::editor
{
editor_view::editor_view(std::string_view title, ui::dock_area* area)
    : ui::dock_window(
          title,
          area,
          ui::dock_window_style{
              .icon_font = &system<ui::ui>().font(ui::DEFAULT_ICON_FONT),
              .title_font = &system<ui::ui>().font(ui::DEFAULT_TEXT_FONT),
              .container_color = ui::COLOR_LIGHT_STEEL_BLUE})
{
}
} // namespace ash::editor