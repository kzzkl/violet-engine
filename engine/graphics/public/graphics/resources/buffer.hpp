#pragma once

#include "graphics/render_device.hpp"

namespace violet
{
class raw_buffer
{
public:
    raw_buffer(const rhi_buffer_desc& desc);
    virtual ~raw_buffer() = default;

    std::size_t get_size() const noexcept
    {
        return m_buffer->get_buffer_size();
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

private:
    rhi_ptr<rhi_buffer> m_buffer;
};

class structured_buffer : public raw_buffer
{
public:
    template <std::ranges::contiguous_range R>
    structured_buffer(
        R&& data,
        rhi_buffer_flags flags,
        rhi_format texel_format = RHI_FORMAT_UNDEFINED)
        : raw_buffer({
              .data = data.data(),
              .size = data.size() * sizeof(decltype(*data.data())),
              .flags = flags,
          })
    {
    }

    structured_buffer(std::size_t buffer_size, rhi_buffer_flags flags);
};

class vertex_buffer : public raw_buffer
{
public:
    template <std::ranges::contiguous_range R>
    vertex_buffer(R&& data, rhi_buffer_flags flags)
        : raw_buffer({
              .data = data.data(),
              .size = data.size() * sizeof(decltype(*data.data())),
              .flags = flags | RHI_BUFFER_VERTEX,
          })
    {
    }

    vertex_buffer(std::size_t buffer_size, rhi_buffer_flags flags);
};

class index_buffer : public raw_buffer
{
public:
    template <std::ranges::contiguous_range R>
    index_buffer(R&& data, rhi_buffer_flags flags)
        : raw_buffer({
              .data = data.data(),
              .size = data.size() * sizeof(decltype(*data.data())),
              .flags = flags | RHI_BUFFER_INDEX,
          })
    {
        m_index_size = sizeof(decltype(*data.data()));
    }

    index_buffer(std::size_t buffer_size, std::size_t index_size, rhi_buffer_flags flags);

    std::size_t get_index_size() const noexcept
    {
        return m_index_size;
    }

private:
    std::size_t m_index_size;
};
} // namespace violet