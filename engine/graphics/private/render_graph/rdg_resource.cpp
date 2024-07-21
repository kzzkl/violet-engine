#include "graphics/render_graph/rdg_resource.hpp"

namespace violet
{
rdg_resource::rdg_resource()
{
}

rdg_resource::~rdg_resource()
{
}

rdg_texture::rdg_texture(
    rhi_texture* texture,
    rhi_texture_layout initial_layout,
    rhi_texture_layout final_layout)
    : m_texture(texture),
      m_initial_layout(initial_layout),
      m_final_layout(final_layout)
{
}

rdg_inter_texture::rdg_inter_texture(
    const rhi_texture_desc& desc,
    rhi_texture_layout initial_layout,
    rhi_texture_layout final_layout)
    : rdg_texture(nullptr, initial_layout, final_layout),
      m_desc(desc)
{
}

rdg_texture_view::rdg_texture_view(
    rhi_texture* texture,
    std::uint32_t level,
    std::uint32_t layer,
    rhi_texture_layout initial_layout,
    rhi_texture_layout final_layout)
    : rdg_texture(texture, initial_layout, final_layout),
      m_rhi_texture(texture),
      m_level(level),
      m_layer(layer)
{
}

rdg_texture_view::rdg_texture_view(
    rdg_texture* texture,
    std::uint32_t level,
    std::uint32_t layer,
    rhi_texture_layout initial_layout,
    rhi_texture_layout final_layout)
    : rdg_texture(texture->get_rhi(), initial_layout, final_layout),
      m_rdg_texture(texture),
      m_level(level),
      m_layer(layer)
{
}

rdg_buffer::rdg_buffer(rhi_buffer* buffer) : m_buffer(buffer)
{
}
} // namespace violet