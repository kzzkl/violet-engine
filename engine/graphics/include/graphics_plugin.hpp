#pragma once

#include "graphics_interface.hpp"
#include "plugin.hpp"

namespace ash::graphics
{
class graphics_plugin : public core::plugin
{
public:
    graphics_plugin();

    factory_interface& factory() { return *m_factory; }

protected:
    virtual bool on_load() override;
    virtual void on_unload() override;

private:
    std::unique_ptr<factory_interface> m_factory;
};
} // namespace ash::graphics