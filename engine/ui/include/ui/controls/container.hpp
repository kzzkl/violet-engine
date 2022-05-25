#pragma once

#include "ui/element_control.hpp"

namespace ash::ui
{
class container : public element_control
{
public:
    container() { m_type = ELEMENT_CONTROL_TYPE_CONTAINER; }
    virtual void extent(const element_extent& extent) override {}
};
} // namespace ash::ui