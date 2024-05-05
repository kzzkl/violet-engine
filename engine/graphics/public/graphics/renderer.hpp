#pragma once

#include "graphics/render_interface.hpp"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace violet
{
class rhi_deleter
{
public:
    rhi_deleter();
    rhi_deleter(rhi* rhi);

    void operator()(rhi_render_pass* render_pass);
    void operator()(rhi_render_pipeline* render_pipeline);
    void operator()(rhi_compute_pipeline* compute_pipeline);
    void operator()(rhi_parameter_layout* parameter_layout);
    void operator()(rhi_parameter* parameter);
    void operator()(rhi_framebuffer* framebuffer);
    void operator()(rhi_sampler* sampler);
    void operator()(rhi_buffer* buffer);
    void operator()(rhi_texture* texture);
    void operator()(rhi_swapchain* swapchain);
    void operator()(rhi_fence* fence);
    void operator()(rhi_semaphore* semaphore);

private:
    rhi* m_rhi;
};

template <typename T>
using rhi_ptr = std::unique_ptr<T, rhi_deleter>;

class renderer
{
public:
    renderer(rhi* rhi);
    ~renderer();

    rhi_render_command* allocate_command();
    void execute(
        const std::vector<rhi_render_command*>& commands,
        const std::vector<rhi_semaphore*>& signal_semaphores,
        const std::vector<rhi_semaphore*>& wait_semaphores,
        rhi_fence* fence);

    void begin_frame();
    void end_frame();

    rhi_fence* get_in_flight_fence();

    std::size_t get_frame_resource_count() const noexcept;
    std::size_t get_frame_resource_index() const noexcept;

    rhi_parameter_layout* add_parameter_layout(
        std::string_view name,
        const std::vector<rhi_parameter_layout_pair>& layout);
    rhi_parameter_layout* get_parameter_layout(std::string_view name) const;

public:
    rhi_ptr<rhi_render_pass> create_render_pass(const rhi_render_pass_desc& desc);

    rhi_ptr<rhi_render_pipeline> create_render_pipeline(const rhi_render_pipeline_desc& desc);
    rhi_ptr<rhi_compute_pipeline> create_compute_pipeline(const rhi_compute_pipeline_desc& desc);

    rhi_ptr<rhi_parameter_layout> create_parameter_layout(const rhi_parameter_layout_desc& desc);
    rhi_ptr<rhi_parameter> create_parameter(rhi_parameter_layout* layout);
    rhi_ptr<rhi_framebuffer> create_framebuffer(const rhi_framebuffer_desc& desc);

    rhi_ptr<rhi_buffer> create_buffer(const rhi_buffer_desc& desc);
    rhi_ptr<rhi_sampler> create_sampler(const rhi_sampler_desc& desc);
    rhi_ptr<rhi_texture> create_texture(const rhi_texture_desc& desc);
    rhi_ptr<rhi_texture> create_texture(const char* file, const rhi_texture_desc& desc = {});
    rhi_ptr<rhi_texture> create_texture_cube(
        std::string_view right,
        std::string_view left,
        std::string_view top,
        std::string_view bottom,
        std::string_view front,
        std::string_view back,
        const rhi_texture_desc& desc = {});

    rhi_ptr<rhi_swapchain> create_swapchain(const rhi_swapchain_desc& desc);

    rhi_ptr<rhi_fence> create_fence(bool signaled);
    rhi_ptr<rhi_semaphore> create_semaphore();

    rhi_deleter& get_deleter() noexcept { return m_rhi_deleter; }

private:
    rhi* m_rhi;
    rhi_deleter m_rhi_deleter;

    std::map<std::string, rhi_ptr<rhi_parameter_layout>> m_parameter_layouts;
};
} // namespace violet