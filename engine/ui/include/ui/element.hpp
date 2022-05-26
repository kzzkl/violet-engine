#pragma once

#include "ui/element_control.hpp"
#include "ui/element_layout.hpp"

namespace ash::ui
{
enum element_state
{
    ELEMENT_STATE_DEFAULT = 0x00,     // 00
    ELEMENT_STATE_MOUSE_ENTER = 0x01, // 01
    ELEMENT_STATE_MOUSE_EXIT = 0x02,  // 10
    ELEMENT_STATE_HOVER = 0x03        // 11
};

struct element
{
    element_layout layout;
    std::unique_ptr<element_control> control;

    element_state state;
    bool focused;

    bool show;
};
} // namespace ash::ui