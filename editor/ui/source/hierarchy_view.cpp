#include "editor/hierarchy_view.hpp"
#include "core/context.hpp"
#include "core/relation.hpp"
#include "ui/controls/panel.hpp"
#include "ui/element.hpp"

namespace ash::editor
{
hierarchy_view::hierarchy_view() : ui::panel(ui::COLOR_BLUE_VIOLET)
{
    show = true;
}

hierarchy_view::~hierarchy_view()
{
}
} // namespace ash::editor