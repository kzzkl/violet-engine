#pragma once

#include "core/context/engine_module.hpp"
#include "interface/graphics_interface.hpp"

namespace violet
{
class rhi_plugin;
class graphics : public engine_module
{
public:
    graphics();
    virtual ~graphics();

    virtual bool initialize(const dictionary& config) override;

    void render();

    rhi_interface* get_rhi() const;

private:
    std::unique_ptr<rhi_plugin> m_plugin;
};
} // namespace violet