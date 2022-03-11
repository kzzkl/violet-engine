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

    renderer* get_renderer() { return m_context->get_renderer(); }
    factory* get_factory() { return m_context->get_factory(); }

protected:
    virtual bool do_load() override;
    virtual void do_unload() override;

private:
    std::unique_ptr<context> m_context;
};
} // namespace ash::graphics