#include "graphics/resources/buffer.hpp"

namespace violet
{
raw_buffer::raw_buffer(const rhi_buffer_desc& desc)
{
    m_buffer = render_device::instance().create_buffer(desc);
}

structured_buffer::structured_buffer(std::size_t size, rhi_buffer_flags flags)
    : raw_buffer({
          .size = size,
          .flags = flags,
      })
{
}
} // namespace violet