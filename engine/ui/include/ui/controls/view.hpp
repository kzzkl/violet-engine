#pragma once

#include "ui/element.hpp"

namespace ash::ui
{
class view : public element
{
public:
    view();

    virtual const element_mesh* mesh() const noexcept override { return &m_mesh; }

private:
    element_mesh m_mesh;
};
} // namespace ash::ui