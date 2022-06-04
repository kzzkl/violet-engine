#pragma once

#include "gallery_control.hpp"
#include "ui/controls/button.hpp"

namespace ash::sample
{
class button_page : public page
{
public:
    button_page();

private:
    void initialize_sample_button();

    std::unique_ptr<text_title_1> m_title;
    std::unique_ptr<text_content> m_description;
    std::vector<std::unique_ptr<display_panel>> m_display;

    std::unique_ptr<text_title_2> m_button_title;
    std::unique_ptr<ui::button> m_button;
    std::size_t m_click_counter;
    std::unique_ptr<ui::label> m_button_text;

    std::unique_ptr<text_title_2> m_icon_button_title;
    std::unique_ptr<ui::icon_button> m_icon_button;
    std::unique_ptr<ui::label> m_icon_button_text;
};
} // namespace ash::sample