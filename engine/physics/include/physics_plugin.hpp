#pragma once

#include "physics_interface.hpp"
#include "plugin.hpp"

namespace ash::physics
{
class physics_plugin : public core::plugin
{
public:
    factory* factory() { return m_context->factory(); }

    void debug(debug_draw* drawer) { m_context->debug(drawer); }

protected:
    virtual bool do_load() override;
    virtual void do_unload() override;

private:
    std::unique_ptr<context> m_context;
};
} // namespace ash::physics