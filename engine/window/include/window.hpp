#pragma once

#include "submodule.hpp"
#include "window_impl.hpp"

namespace ash::window
{
class WINDOW_API window : public ash::core::submodule
{
public:
    static constexpr uuid id = "cc5fc9c8-cecb-4a46-bc4c-9bc094fdb463";

public:
    window();

    virtual bool initialize(const dictionary& config) override;

    mouse& get_mouse() { return m_impl->get_mouse(); }

    const void* get_handle() const { return m_impl->get_handle(); }
    window_rect get_rect() const { return m_impl->get_rect(); }

    void set_title(std::string_view title) { m_impl->set_title(title); }

private:
    std::unique_ptr<window_impl> m_impl;
};
} // namespace ash::window