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

    void set_light(rhi_parameter* light) noexcept { m_light = light; }
    rhi_parameter* get_light() const noexcept { return m_light; }

    void set_camera(rhi_parameter* camera) noexcept { m_camera = camera; }
    rhi_parameter* get_camera() const noexcept { return m_camera; }

    void set_resource(std::string_view name, rhi_texture* texture);
    void set_resource(std::string_view name, rhi_buffer* buffer);
    void set_resource(std::string_view name, rhi_swapchain* swapchain);

    rhi_framebuffer* get_framebuffer(const std::vector<std::size_t>& resource_indices);

private:
    rhi_parameter* m_light;
    rhi_parameter* m_camera;
};
} // namespace violet