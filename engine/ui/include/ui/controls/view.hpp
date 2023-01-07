#pragma once

#include "ui/element.hpp"

namespace violet::ui
{
class view : public element
{
public:
    view();

    virtual const element_mesh* mesh() const noexcept override { return &m_mesh; }

private:
    element_mesh m_mesh;
};
} // namespace violet::ui