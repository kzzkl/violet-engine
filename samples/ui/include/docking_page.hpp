#pragma once

#include "gallery_control.hpp"
#include "ui/controls/dock_panel.hpp"
#include "ui/controls/button.hpp"

namespace ash::sample
{
class docking_page : public page
{
public:
    docking_page();

private:
    void initialize_sample_docking();

    std::unique_ptr<text_title_1> m_title;
    std::unique_ptr<text_content> m_description;
    std::vector<std::unique_ptr<display_panel>> m_display;

    std::unique_ptr<ui::dock_panel> m_root;
    std::unique_ptr<ui::element> m_right;
    std::vector<std::unique_ptr<ui::dock_panel>> m_dock_panels;

    std::unique_ptr<ui::button> m_button;
    std::unique_ptr<ui::button> m_button2;

    float m_width;
};
} // namespace ash::sample
