#pragma once

#include "submodule.hpp"
#include "window_impl.hpp"

namespace ash::window
{
class WINDOW_API window : public ash::core::submodule
{
public:
    window();

    virtual bool initialize(const ash::common::dictionary& config) override;

    mouse& get_mouse() { return m_impl->get_mouse(); }

private:
    std::unique_ptr<window_impl> m_impl;
};
} // namespace ash::window