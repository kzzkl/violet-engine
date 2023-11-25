#pragma once

#include "core/plugin.hpp"
#include "physics/physics_interface.hpp"

namespace violet
{
class physics_plugin : public plugin
{
public:
    physics_plugin();

    pei_plugin* get_pei() { return m_pei; }

protected:
    virtual bool on_load() override;
    virtual void on_unload() override;

private:
    create_pei m_create_func;
    destroy_pei m_destroy_func;

    pei_plugin* m_pei;
};
} // namespace violet