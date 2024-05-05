#include "graphics/render_graph/render_context.hpp"

namespace violet
{
render_context::render_context()
{
}

void render_context::set_resource(std::string_view name, rhi_texture* texture)
{
}

void render_context::set_resource(std::string_view name, rhi_buffer* buffer)
{
}

void render_context::set_resource(std::string_view name, rhi_swapchain* swapchain)
{
}

rhi_framebuffer* render_context::get_framebuffer(const std::vector<std::size_t>& resource_indices)
{
    return nullptr;
}
} // namespace violet