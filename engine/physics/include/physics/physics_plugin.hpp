#pragma once

#include "core/plugin.hpp"
#include "physics_interface.hpp"

namespace ash::physics
{
class physics_plugin : public core::plugin
{
public:
    physics_plugin();
    factory_interface& factory() { return *m_factory; }

protected:
    virtual bool on_load() override;
    virtual void on_unload() override;

private:
    std::unique_ptr<factory_interface> m_factory;
};
} // namespace ash::physics