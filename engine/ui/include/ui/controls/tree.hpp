#pragma once

#include "ui/controls/label.hpp"
#include "ui/controls/panel.hpp"
#include "ui/element.hpp"
#include <string>

namespace ash::ui
{
class tree_node : public panel
{
public:
    tree_node(
        std::string_view text,
        const font& font,
        std::uint32_t color = COLOR_BLACK,
        std::uint32_t background = COLOR_WHITE);

    label* title() const noexcept { return m_label.get(); }
    element* container() const noexcept { return m_container.get(); }

private:
    std::unique_ptr<label> m_label;
    std::unique_ptr<element> m_container;
};

class tree : public element
{
public:
    tree();

private:
};
} // namespace ash::ui