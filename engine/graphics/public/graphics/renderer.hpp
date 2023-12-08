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
    rhi_deleter(rhi_renderer* rhi);

    void operator()(rhi_render_pass* render_pass);
    void operator()(rhi_render_pipeline* render_pipeline);
    void operator()(rhi_compute_pipeline* compute_pipeline);
    void operator()(rhi_parameter_layout* parameter_layout);
    void operator()(rhi_parameter* parameter);
    void operator()(rhi_framebuffer* framebuffer);
    void operator()(rhi_sampler* sampler);
    void operator()(rhi_resource* resource);
    void operator()(rhi_fence* fence);
    void operator()(rhi_semaphore* semaphore);

private:
    rhi_renderer* m_rhi;
};

template <typename T>
using rhi_ptr = std::unique_ptr<T, rhi_deleter>;

class renderer
{
public:
    renderer(rhi_renderer* rhi);
    ~renderer();

    rhi_render_command* allocate_command();
    void execute(
        const std::vector<rhi_render_command*>& commands,
        const std::vector<rhi_semaphore*>& signal_semaphores,
        const std::vector<rhi_semaphore*>& wait_semaphores,
        rhi_fence* fence);

    void begin_frame();
    void end_frame();
    void present(const std::vector<rhi_semaphore*>& wait_semaphores);

    void resize(std::uint32_t width, std::uint32_t height);

    rhi_resource* get_back_buffer();
    rhi_fence* get_in_flight_fence();
    rhi_semaphore* get_image_available_semaphore();

    std::size_t get_frame_resource_count() const noexcept;
    std::size_t get_frame_resource_index() const noexcept;

    rhi_parameter_layout* add_parameter_layout(
        std::string_view name,
        const std::vector<rhi_parameter_layout_pair>& layout);
    rhi_parameter_layout* get_parameter_layout(std::string_view name) const;

    rhi_parameter* get_light_parameter() const noexcept { return m_light_parameter.get(); }

public:
    rhi_ptr<rhi_render_pass> create_render_pass(const rhi_render_pass_desc& desc);

    rhi_ptr<rhi_render_pipeline> create_render_pipeline(const rhi_render_pipeline_desc& desc);
    rhi_ptr<rhi_compute_pipeline> create_compute_pipeline(const rhi_compute_pipeline_desc& desc);

    rhi_ptr<rhi_parameter_layout> create_parameter_layout(const rhi_parameter_layout_desc& desc);
    rhi_ptr<rhi_parameter> create_parameter(rhi_parameter_layout* layout);
    rhi_ptr<rhi_framebuffer> create_framebuffer(const rhi_framebuffer_desc& desc);

    rhi_ptr<rhi_resource> create_buffer(const rhi_buffer_desc& desc);
    rhi_ptr<rhi_sampler> create_sampler(const rhi_sampler_desc& desc);
    rhi_ptr<rhi_resource> create_texture(
        const std::uint8_t* data,
        std::uint32_t width,
        std::uint32_t height,
        rhi_resource_format format = RHI_RESOURCE_FORMAT_R8G8B8A8_UNORM);
    rhi_ptr<rhi_resource> create_texture(const char* file);
    rhi_ptr<rhi_resource> create_texture_cube(
        std::string_view left,
        std::string_view right,
        std::string_view top,
        std::string_view bottom,
        std::string_view front,
        std::string_view back);

    rhi_ptr<rhi_resource> create_render_target(const rhi_render_target_desc& desc);
    rhi_ptr<rhi_resource> create_depth_stencil_buffer(const rhi_depth_stencil_buffer_desc& desc);

    rhi_ptr<rhi_fence> create_fence(bool signaled);
    rhi_ptr<rhi_semaphore> create_semaphore();

    rhi_deleter& get_deleter() noexcept { return m_rhi_deleter; }

private:
    rhi_renderer* m_rhi;
    rhi_deleter m_rhi_deleter;

    std::map<std::string, rhi_ptr<rhi_parameter_layout>> m_parameter_layouts;

    rhi_ptr<rhi_parameter> m_light_parameter;
};
} // namespace violet