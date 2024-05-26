#pragma once

#include "ui/rendering/ui_pass.hpp"
#include "ui/widget.hpp"
#include <memory>

namespace violet
{
class ui_root
{
public:
    ui_root();

    widget* get_container() const noexcept { return m_container.get(); }

    void set_pass(ui_pass* pass) noexcept { m_pass = pass; }
    ui_pass* get_pass() const noexcept { return m_pass; }

private:
    std::unique_ptr<widget> m_container;
    ui_pass* m_pass;
};
} // namespace violet