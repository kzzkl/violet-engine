#pragma once

#include "graphics_interface.hpp"
#include "plugin.hpp"

namespace ash::graphics
{
class graphics_plugin : public ash::core::plugin
{
public:
    graphics_plugin();

    bool initialize(const context_config& config);

    renderer* renderer() { return m_context->renderer(); }
    factory* factory() { return m_context->factory(); }

protected:
    virtual bool do_load() override;
    virtual void do_unload() override;

private:
    std::unique_ptr<context> m_context;
};
} // namespace ash::graphics