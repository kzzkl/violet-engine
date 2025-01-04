#pragma once

#include "core/engine.hpp"
#include "graphics/render_device.hpp"

namespace violet
{
class imgui_system : public system
{
public:
    imgui_system();

    bool initialize(const dictionary& config) override;

private:
    void initialize_font();

    void begin_frame();
    void end_frame();

    rhi_ptr<rhi_texture> m_font;
};
} // namespace violet