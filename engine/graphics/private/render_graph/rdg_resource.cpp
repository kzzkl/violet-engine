#include "graphics/render_graph/rdg_resource.hpp"

namespace violet
{
rdg_resource::rdg_resource(bool external) : m_external(external)
{
}

rdg_resource::~rdg_resource()
{
}

rdg_texture::rdg_texture(
    rhi_texture* texture,
    rhi_texture_layout initial_layout,
    rhi_texture_layout final_layout)
    : rdg_resource(true),
      m_texture(texture),
      m_initial_layout(initial_layout),
      m_final_layout(final_layout)
{
    m_desc.format = m_texture->get_format();
    m_desc.samples = m_texture->get_samples();
    m_desc.extent = m_texture->get_extent();
}

rdg_texture::rdg_texture(
    const rhi_texture_desc& desc,
    rhi_texture_layout initial_layout,
    rhi_texture_layout final_layout)
    : rdg_resource(false),
      m_desc(desc),
      m_initial_layout(initial_layout),
      m_final_layout(final_layout)
{
}

rdg_buffer::rdg_buffer(rhi_buffer* buffer) : rdg_resource(buffer != nullptr), m_buffer(buffer)
{
}
} // namespace violet