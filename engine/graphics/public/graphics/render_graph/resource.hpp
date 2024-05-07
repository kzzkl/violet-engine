#pragma once

#include "graphics/render_interface.hpp"
#include <string>

namespace violet
{
class resource
{
public:
    resource(std::string_view name, bool external = false);
    virtual ~resource();

    const std::string& get_name() const noexcept { return m_name; }
    bool is_external() const noexcept { return m_external; }

    virtual rhi_texture* get_texture() { return nullptr; }
    virtual rhi_buffer* get_buffer() { return nullptr; }

private:
    std::string m_name;
    bool m_external;
};

class texture : public resource
{
public:
    texture(std::string_view name, bool external = false);

    void set_format(rhi_format format) noexcept { m_format = format; }
    rhi_format get_format() const noexcept { return m_format; }

    void set_samples(rhi_sample_count samples) noexcept { m_samples = samples; }
    rhi_sample_count get_samples() const noexcept { return m_samples; }

    void set(rhi_texture* texture) noexcept { m_texture = texture; }

    virtual rhi_texture* get_texture() { return m_texture; }

private:
    rhi_format m_format;
    rhi_sample_count m_samples;

    rhi_texture* m_texture;
};

class swapchain : public resource
{
public:
    swapchain(std::string_view name);

    void set_format(rhi_format format) noexcept { m_format = format; }
    rhi_format get_format() const noexcept { return m_format; }

    void set_samples(rhi_sample_count samples) noexcept { m_samples = samples; }
    rhi_sample_count get_samples() const noexcept { return m_samples; }

    rhi_semaphore* acquire_texture() { return m_swapchain->acquire_texture(); }
    void present(rhi_semaphore* wait_semaphore) { m_swapchain->present(&wait_semaphore, 1); }

    void set(rhi_swapchain* swapchain) noexcept { m_swapchain = swapchain; }

    virtual rhi_texture* get_texture() { return m_swapchain->get_texture(); }

private:
    rhi_format m_format;
    rhi_sample_count m_samples;

    rhi_swapchain* m_swapchain;
};

class buffer : public resource
{
public:
    using resource::resource;
};
} // namespace violet