#include "graphics/render_graph/rdg_resource.hpp"
#include <cassert>

namespace violet
{
bool rdg_reference::is_first_reference() const noexcept
{
    return resource->get_references().front() == this;
}

bool rdg_reference::is_last_reference() const noexcept
{
    return resource->get_references().back() == this;
}

rdg_reference* rdg_reference::get_prev_reference() const
{
    assert(!is_first_reference());
    return resource->get_references()[index - 1];
}

rdg_reference* rdg_reference::get_next_reference() const
{
    assert(!is_last_reference());
    return resource->get_references()[index + 1];
}

rdg_resource::rdg_resource() {}

rdg_resource::~rdg_resource() {}

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

rdg_buffer::rdg_buffer(rhi_buffer* buffer)
    : m_buffer(buffer)
{
}

rdg_inter_buffer::rdg_inter_buffer(const rhi_buffer_desc& desc)
    : m_desc(desc)
{
}
} // namespace violet