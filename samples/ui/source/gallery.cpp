#include "gallery.hpp"
#include "ui/widgets/node_editor.hpp"
#include "ui/widgets/panel.hpp"

namespace violet
{
gallery::gallery()
{
    widget_layout* layout = get_layout();
    layout->set_flex_direction(LAYOUT_FLEX_DIRECTION_ROW);
    layout->set_flex_grow(1.0f);

    // panel* navi_panel = add<panel>();
    // navi_panel->get_layout()->set_width(300.0f);
    // navi_panel->set_color(ui_color::BISQUE);

    panel* main_panel = add<panel>();
    main_panel->get_layout()->set_flex_grow(1.0f);
    main_panel->set_color(ui_color::BEIGE);

    main_panel->add<node_editor>();
}
} // namespace violet