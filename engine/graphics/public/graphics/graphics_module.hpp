#pragma once

#include "core/context/engine_module.hpp"
#include "graphics/rhi.hpp"

namespace violet
{
class rhi_plugin;
class graphics_module : public engine_module
{
public:
    graphics_module();
    virtual ~graphics_module();

    virtual bool initialize(const dictionary& config) override;

    void render();

    rhi_context* get_rhi() const;

private:
    std::unique_ptr<rhi_plugin> m_plugin;
};
} // namespace violet