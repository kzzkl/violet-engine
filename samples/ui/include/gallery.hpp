#pragma once

#include "tree_page.hpp"
#include "ui/controls/label.hpp"
#include "ui/controls/panel.hpp"
#include "ui/controls/scroll_view.hpp"

namespace ash::sample
{
class gallery
{
public:
    void initialize();

private:
    std::unique_ptr<ui::panel> m_left;

    std::unique_ptr<tree_page> m_tree_page;
};
} // namespace ash::sample