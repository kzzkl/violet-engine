#pragma once

#include "ui/element.hpp"
#include <string>

namespace ash::ui
{
class tree_node : public element
{
public:
    tree_node();

    virtual void tick() override;

private:
    bool m_open;
};

class tree : public element
{
public:
    tree();

private:
};
} // namespace ash::ui