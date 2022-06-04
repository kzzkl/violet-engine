#pragma once

#include "button_page.hpp"
#include "tree_page.hpp"
#include "ui/controls/scroll_view.hpp"
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

    std::unique_ptr<ui::panel> m_left;
    std::unique_ptr<ui::tree> m_navigation_tree;
    ui::tree_node_style m_navigation_node_style;

    std::unique_ptr<ui::scroll_view> m_main_view;

    page* m_current_page;
    std::map<std::string, std::unique_ptr<page>> m_pages;

    // Basic,
    std::unique_ptr<ui::tree_node> m_basic_node;
    std::unique_ptr<ui::tree_node> m_button_page_node;

    // Views.
    std::unique_ptr<ui::tree_node> m_views_node;
    std::unique_ptr<ui::tree_node> m_tree_page_node;
};
} // namespace ash::sample