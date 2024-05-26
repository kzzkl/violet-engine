#pragma once

#include "ui/control.hpp"

namespace violet
{
class font;
class font_icon;
class label;
class panel;

struct collapse_theme
{
    const font* icon_font;
    std::uint32_t icon_color;
    float icon_scale;

    const font* title_font;
    std::uint32_t title_color;

    std::uint32_t bar_color;
    std::uint32_t container_color;
};

class collapse : public control
{
public:
    collapse(std::string_view title, const collapse_theme& theme);
    collapse(std::string_view title, std::uint32_t icon, const collapse_theme& theme);
    virtual ~collapse();

    void add_item(control* item);
    void remove_item(control* item);

    bool is_open() const noexcept;

    panel* container() const noexcept { return m_container.get(); }

private:
    std::unique_ptr<font_icon> m_icon;
    std::unique_ptr<label> m_title;
    std::unique_ptr<panel> m_tab;

    std::unique_ptr<panel> m_container;
};
} // namespace violet