#pragma once

#include "page.hpp"
#include "ui/controls/tree.hpp"

namespace violet::sample
{
class tree_page : public page
{
public:
    tree_page();

private:
    void initialize_sample_tree();

    std::unique_ptr<ui::tree> m_tree;

    std::vector<std::unique_ptr<ui::tree_node>> m_chapters;
    std::vector<std::unique_ptr<ui::tree_node>> m_items;
};
} // namespace violet::sample