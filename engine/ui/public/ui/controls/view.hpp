#pragma once

#include "ui/control.hpp"

namespace violet::ui
{
class view : public control
{
public:
    view();

    virtual const control_mesh* mesh() const noexcept override { return &m_mesh; }

private:
    control_mesh m_mesh;
};
} // namespace violet::ui