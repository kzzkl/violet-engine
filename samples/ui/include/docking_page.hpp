#pragma once

#include "gallery_control.hpp"
#include "ui/controls/button.hpp"
#include "ui/controls/dock_area.hpp"
#include "ui/controls/dock_window.hpp"

namespace ash::sample
{
class docking_page : public page
{
public:
    docking_page();

private:
    void initialize_sample_docking();
    ui::dock_element* make_dock_window();

    std::unique_ptr<text_title_1> m_title;
    std::unique_ptr<text_content> m_description;
    std::vector<std::unique_ptr<display_panel>> m_display;

    std::unique_ptr<ui::dock_area> m_dock_area;
    std::vector<std::unique_ptr<ui::dock_window>> m_dock_windows;
    std::vector<std::unique_ptr<ui::label>> m_window_labels;

    std::unique_ptr<ui::element> m_right;
    std::unique_ptr<ui::button> m_create_button;
    std::unique_ptr<ui::button> m_print_button;
    std::unique_ptr<ui::button> m_test_button;
};
} // namespace ash::sample
