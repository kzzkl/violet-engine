#pragma once

#include "page.hpp"
#include "ui/controls/button.hpp"
#include "ui/controls/dock_area.hpp"
#include "ui/controls/dock_window.hpp"

namespace violet::sample
{
class docking_page : public page
{
public:
    docking_page();

private:
    void initialize_sample_docking();
    ui::dock_element* make_dock_window();

    std::unique_ptr<ui::dock_area> m_dock_area;
    std::vector<std::unique_ptr<ui::dock_window>> m_dock_windows;
    std::vector<std::unique_ptr<ui::label>> m_window_labels;

    std::unique_ptr<ui::element> m_right;
    std::unique_ptr<ui::button> m_create_button;
    std::unique_ptr<ui::button> m_print_button;
};
} // namespace violet::sample
