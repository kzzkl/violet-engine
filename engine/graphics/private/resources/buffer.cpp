#include "graphics/resources/buffer.hpp"

namespace violet
{
raw_buffer::raw_buffer() = default;

raw_buffer::raw_buffer(const void* data, std::size_t size, rhi_buffer_flags flags)
{
    set_buffer({
        .data = data,
        .size = size,
        .flags = flags,
    });
}

void raw_buffer::set_buffer(const rhi_buffer_desc& desc)
{
    m_buffer = render_device::instance().create_buffer(desc);
}

void raw_buffer::set_buffer(rhi_ptr<rhi_buffer>&& buffer)
{
    m_buffer = std::move(buffer);
}

structured_buffer::structured_buffer(std::size_t buffer_size, rhi_buffer_flags flags)
{
    set_buffer({
        .size = buffer_size,
        .flags = flags,
    });
}
} // namespace violet