#pragma once

#include "graphics/render_device.hpp"

namespace violet
{
class raw_buffer
{
public:
    raw_buffer();
    raw_buffer(const void* data, std::size_t size, rhi_buffer_flags flags);
    raw_buffer(const raw_buffer&) = delete;
    virtual ~raw_buffer() = default;

    raw_buffer& operator=(const raw_buffer&) = delete;

    std::size_t get_size() const noexcept
    {
        return m_buffer->get_size();
    }

    rhi_buffer_srv* get_srv(
        std::size_t offset = 0,
        std::size_t size = 0,
        rhi_format format = RHI_FORMAT_UNDEFINED) const noexcept
    {
        return m_buffer->get_srv(offset, size, format);
    }

    rhi_buffer_uav* get_uav(
        std::size_t offset = 0,
        std::size_t size = 0,
        rhi_format format = RHI_FORMAT_UNDEFINED) const noexcept
    {
        return m_buffer->get_uav(offset, size, format);
    }

    rhi_buffer* get_rhi() const noexcept
    {
        return m_buffer.get();
    }

    void* get_buffer_pointer() const noexcept
    {
        return m_buffer->get_buffer_pointer();
    }

protected:
    void set_buffer(const rhi_buffer_desc& desc);
    void set_buffer(rhi_ptr<rhi_buffer>&& buffer);

private:
    rhi_ptr<rhi_buffer> m_buffer;
};

class structured_buffer : public raw_buffer
{
public:
    template <std::ranges::contiguous_range R>
    structured_buffer(R&& data, rhi_buffer_flags flags)
    {
        set_buffer({
            .data = data.data(),
            .size = data.size() * sizeof(decltype(*data.data())),
            .flags = flags,
        });
    }

    structured_buffer(std::size_t buffer_size, rhi_buffer_flags flags);
};
} // namespace violet