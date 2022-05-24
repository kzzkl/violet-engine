#pragma once

#include <memory>

namespace ash::ui
{
class element_layout
{
public:
private:
};

class layout_impl
{
};

class layout
{
public:
    layout();

    void add_node();

    void calculate();

private:
    std::unique_ptr<layout_impl> m_impl;
};
} // namespace ash::ui