#pragma once

#include "graphics/render_interface.hpp"
#include <string>
#include <vector>

namespace violet
{
class render_context
{
public:
    render_context();

    void set_camera(rhi_parameter* camera) noexcept { m_camera = camera; }
    rhi_parameter* get_camera() const noexcept { return m_camera; }

    void set_light(rhi_parameter* light) noexcept { m_light = light; }
    rhi_parameter* get_light() const noexcept { return m_light; }

private:
    rhi_parameter* m_camera;
    rhi_parameter* m_light;
};
} // namespace violet