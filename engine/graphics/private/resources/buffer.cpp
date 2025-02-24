#include "graphics/resources/buffer.hpp"

namespace violet
{
raw_buffer::raw_buffer(const rhi_buffer_desc& desc)
{
    m_buffer = render_device::instance().create_buffer(desc);
}

structured_buffer::structured_buffer(std::size_t buffer_size, rhi_buffer_flags flags)
    : raw_buffer({
          .size = buffer_size,
          .flags = flags,
      })
{
}

vertex_buffer::vertex_buffer(std::size_t buffer_size, rhi_buffer_flags flags)
    : raw_buffer({
          .size = buffer_size,
          .flags = flags | RHI_BUFFER_VERTEX,
      })
{
}

index_buffer::index_buffer(std::size_t buffer_size, std::size_t index_size, rhi_buffer_flags flags)
    : raw_buffer({
          .size = buffer_size,
          .flags = flags | RHI_BUFFER_INDEX,
      }),
      m_index_size(index_size)
{
}
} // namespace violet