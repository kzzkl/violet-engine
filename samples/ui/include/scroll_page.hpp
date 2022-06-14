#pragma once

#include "gallery_control.hpp"
#include "ui/controls/scroll_view.hpp"

namespace ash::sample
{
class scroll_page : public page
{
public:
    scroll_page();

private:
    void initialize_sample_scroll();

    std::unique_ptr<text_title_1> m_title;
    std::unique_ptr<text_content> m_description;
    std::vector<std::unique_ptr<display_panel>> m_display;

    std::unique_ptr<ui::scroll_view> m_scroll_view;
    std::vector<std::unique_ptr<ui::panel>> m_panels;
};
} // namespace ash::sample