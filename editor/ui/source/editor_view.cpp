#include "editor/editor_view.hpp"
#include "ui/ui.hpp"

namespace ash::editor
{
editor_view::editor_view(std::string_view title, std::uint32_t icon_index)
    : ui::dock_window(
          title,
          icon_index,
          ui::dock_window_style{
              .icon_font = &system<ui::ui>().font(ui::DEFAULT_ICON_FONT),
              .title_font = &system<ui::ui>().font(ui::DEFAULT_TEXT_FONT),
              .container_color = ui::COLOR_LIGHT_STEEL_BLUE})
{
}
} // namespace ash::editor