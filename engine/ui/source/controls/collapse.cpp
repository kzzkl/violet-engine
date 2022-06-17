#include "ui/controls/collapse.hpp"
#include "ui/controls/font_icon.hpp"
#include "ui/controls/label.hpp"
#include "ui/controls/panel.hpp"

namespace ash::ui
{
collapse::collapse(std::string_view title, const collapse_theme& theme)
{
    flex_direction(LAYOUT_FLEX_DIRECTION_COLUMN);
    m_tab = std::make_unique<panel>(theme.bar_color);
    m_tab->flex_direction(LAYOUT_FLEX_DIRECTION_ROW);
    m_tab->flex_wrap(LAYOUT_FLEX_WRAP_NOWRAP);
    m_tab->align_items(LAYOUT_ALIGN_CENTER);
    m_tab->padding(10.0f, LAYOUT_EDGE_HORIZONTAL);
    m_tab->padding(3.0f, LAYOUT_EDGE_VERTICAL);
    m_tab->on_mouse_press = [this](window::mouse_key key, int x, int y) -> bool {
        if (key == window::MOUSE_KEY_LEFT)
        {
            if (m_container->display())
                m_container->hide();
            else
                m_container->show();
        }
        return false;
    };
    add(m_tab.get());

    label_theme title_theme = {};
    title_theme.text_font = theme.title_font;
    title_theme.text_color = theme.title_color;
    m_title = std::make_unique<label>(title, title_theme);
    m_tab->add(m_title.get());

    m_container = std::make_unique<panel>(theme.container_color);
    m_container->width_percent(100.0f);
    m_container->hide();
    add(m_container.get());
}

collapse::collapse(std::string_view title, std::uint32_t icon, const collapse_theme& theme)
    : collapse(title, theme)
{
    font_icon_theme icon_theme = {};
    icon_theme.icon_font = theme.icon_font;
    icon_theme.icon_color = theme.icon_color;
    icon_theme.icon_scale = theme.icon_scale;
    m_icon = std::make_unique<font_icon>(icon, icon_theme);
    m_icon->margin(5.0f, LAYOUT_EDGE_RIGHT);
    m_tab->add(m_icon.get(), 0);
}

collapse::~collapse()
{
}

void collapse::add_item(element* item)
{
    m_container->add(item);
}

void collapse::remove_item(element* item)
{
    m_container->remove(item);
}
} // namespace ash::ui