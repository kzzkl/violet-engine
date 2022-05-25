#pragma once

#include "ui/element_control.hpp"
#include "ui/element_layout.hpp"

namespace ash::ui
{
struct element
{
    element_layout layout;
    std::unique_ptr<element_control> control;

    bool show;
};
} // namespace ash::ui