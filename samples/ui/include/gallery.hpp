#pragma once

#include "gallery_control.hpp"
#include "ui/controls/scroll_view.hpp"
#include "ui/controls/tree.hpp"
#include <map>

namespace ash::sample
{
class gallery
{
public:
    gallery();

    void initialize();

private:
    void initialize_basic();
    void initialize_views();
    void initialize_docking();

    std::unique_ptr<ui::panel> m_left;
    std::unique_ptr<ui::tree> m_navigation_tree;
    ui::tree_node_style m_navigation_node_style;

    std::unique_ptr<ui::scroll_view> m_main_view;

    page* m_current_page;
    std::map<std::string, std::unique_ptr<page>> m_pages;
    std::map<std::string, std::unique_ptr<ui::tree_node>> m_nodes;
};
} // namespace ash::sample