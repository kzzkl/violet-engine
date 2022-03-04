#pragma once

#include "graphics_interface.hpp"
#include "plugin.hpp"

namespace ash::graphics
{
class graphics_plugin : public ash::core::plugin
{
public:
    using context = ash::graphics::external::graphics_context;

public:
    graphics_plugin();

    context* get_context() { return m_context.get(); }

protected:
    virtual bool do_load() override;
    virtual void do_unload() override;

private:
    std::unique_ptr<context> m_context;
};
} // namespace ash::graphics