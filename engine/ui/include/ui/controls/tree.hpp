#pragma once

#include "ui/controls/container.hpp"
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

    virtual void tick() override;

    label* node_label() const noexcept { return m_label.get(); }
    container* node_container() const noexcept { return m_container.get(); }

private:
    std::unique_ptr<label> m_label;
    std::unique_ptr<container> m_container;
};

class tree : public element
{
public:
    tree();

private:
};
} // namespace ash::ui