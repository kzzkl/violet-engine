#pragma once

#include "page.hpp"
#include "ui/controls/scroll_view.hpp"
#include "ui/controls/tree.hpp"
#include <map>

namespace violet::sample
{
class gallery
{
public:
    gallery();

    void initialize();

private:
    void initialize_theme();
    void initialize_navigation();

    std::unique_ptr<ui::panel> m_left;
    std::unique_ptr<ui::tree> m_navigation_tree;

    std::unique_ptr<ui::scroll_view> m_main_view;

    page* m_current_page;
    std::map<std::string, std::unique_ptr<page>> m_pages;
    std::map<std::string, std::unique_ptr<ui::tree_node>> m_nodes;
};
} // namespace violet::sample