#pragma once

#include "page.hpp"
#include "ui/controls/button.hpp"

namespace violet::sample
{
class button_page : public page
{
public:
    button_page();

private:
    void initialize_sample_button();

    std::unique_ptr<ui::button> m_button;
    std::size_t m_click_counter;
    std::unique_ptr<ui::label> m_button_text;

    std::unique_ptr<ui::icon_button> m_icon_button;
    std::unique_ptr<ui::label> m_icon_button_text;
};
} // namespace violet::sample