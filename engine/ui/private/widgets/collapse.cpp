#include "ui/widgets/collapse.hpp"
#include "ui/widgets/font_icon.hpp"
#include "ui/widgets/label.hpp"
#include "ui/widgets/panel.hpp"

namespace violet
{
collapse::collapse(std::string_view title, const collapse_theme& theme)
{
    layout()->set_flex_direction(LAYOUT_FLEX_DIRECTION_COLUMN);
    m_tab = std::make_unique<panel>(theme.bar_color);
    m_tab->layout()->set_flex_direction(LAYOUT_FLEX_DIRECTION_ROW);
    m_tab->layout()->set_flex_wrap(LAYOUT_FLEX_WRAP_NOWRAP);
    m_tab->layout()->set_align_items(LAYOUT_ALIGN_CENTER);
    m_tab->layout()->set_padding(10.0f, LAYOUT_EDGE_HORIZONTAL);
    m_tab->layout()->set_padding(3.0f, LAYOUT_EDGE_VERTICAL);
    m_tab->event()->on_mouse_press = [this](mouse_key key, int x, int y) -> bool
    {
        if (key == MOUSE_KEY_LEFT)
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
    m_container->layout()->set_width_percent(100.0f);
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
    m_icon->layout()->set_margin(5.0f, LAYOUT_EDGE_RIGHT);
    m_tab->add(m_icon.get(), 0);
}

collapse::~collapse()
{
}

void collapse::add_item(control* item)
{
    m_container->add(item);
}

void collapse::remove_item(control* item)
{
    m_container->remove(item);
}

bool collapse::is_open() const noexcept
{
    return m_container->display();
}
} // namespace violet