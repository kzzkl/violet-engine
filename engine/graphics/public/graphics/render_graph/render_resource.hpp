#pragma once

#include "graphics/render_interface.hpp"

namespace violet
{
class render_resource
{
public:
    render_resource(bool framebuffer_cache = true);

    void set_image(rhi_image* image) noexcept { m_image = image; }
    rhi_image* get_image() const noexcept { return m_image; }

    void set_format(rhi_resource_format format) noexcept { m_format = format; }
    rhi_resource_format get_format() const noexcept { return m_format; }

    void set_samples(rhi_sample_count samples) noexcept { m_samples = samples; }
    rhi_sample_count get_samples() const noexcept { return m_samples; }

    bool is_framebuffer_cache() const noexcept { return m_framebuffer_cache; }

private:
    rhi_resource_format m_format;
    rhi_sample_count m_samples;

    rhi_image* m_image;

    bool m_framebuffer_cache;
};
} // namespace violet