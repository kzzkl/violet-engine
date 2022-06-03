#pragma once

#include "gallery_control.hpp"
#include "ui/controls/tree.hpp"
#include "ui/element.hpp"

namespace ash::sample
{
class tree_page : public page
{
public:
    tree_page();

private:
    void initialize_sample_tree();

    std::unique_ptr<ui::label> m_page_title;
    std::unique_ptr<ui::label> m_page_description;
    std::unique_ptr<ui::label> m_test;

    std::unique_ptr<display_panel> m_display;
    std::unique_ptr<ui::tree> m_tree;

    std::vector<std::unique_ptr<ui::tree_node>> m_chapters;
    std::vector<std::unique_ptr<ui::tree_node>> m_items;

    std::unique_ptr<ui::label> m_icon;
};
} // namespace ash::sample