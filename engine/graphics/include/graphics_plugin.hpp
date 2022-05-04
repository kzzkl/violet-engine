#pragma once

#include "graphics_interface.hpp"
#include "plugin.hpp"

namespace ash::graphics
{
class graphics_plugin : public ash::core::plugin
{
public:
    graphics_plugin();

    factory_interface& factory() { return *m_factory; }

protected:
    virtual bool do_load() override;
    virtual void do_unload() override;

private:
    std::unique_ptr<factory_interface> m_factory;
};
} // namespace ash::graphics