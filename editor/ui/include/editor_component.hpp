#pragma once

#include "editor_data.hpp"
#include "ui.hpp"
#include <memory>

namespace ash::editor
{
class editor_view
{
public:
    virtual ~editor_view() = default;

    virtual void draw(ui::ui& ui, editor_data& data) = 0;
};

struct editor_ui
{
    bool show;
    std::unique_ptr<editor_view> interface;
};
} // namespace ash::editor