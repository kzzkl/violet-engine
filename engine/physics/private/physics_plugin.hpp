#pragma once

#include "core/plugin.hpp"
#include "physics/physics_interface.hpp"

namespace violet
{
class physics_plugin : public plugin
{
public:
    physics_plugin();

    phy_plugin* get_plugin() { return m_plugin; }

protected:
    virtual bool on_load() override;
    virtual void on_unload() override;

private:
    phy_create_plugin m_create_func;
    phy_destroy_plugin m_destroy_func;

    phy_plugin* m_plugin;
};
} // namespace violet