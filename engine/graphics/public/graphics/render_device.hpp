#pragma once

#include "common/hash.hpp"
#include "graphics/render_interface.hpp"
#include "math/math.hpp"
#include <cassert>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>

namespace violet
{
template <typename T>
concept has_parameter = std::is_array_v<decltype(std::declval<T>().parameters)> &&
                        std::is_same_v<
                            std::remove_all_extents_t<decltype(std::declval<T>().parameters)>,
                            const rhi_shader_desc::parameter_slot>;

template <typename T>
concept has_input = std::is_array_v<decltype(std::declval<T>().inputs)> &&
                    std::is_same_v<
                        std::remove_all_extents_t<decltype(std::declval<T>().inputs)>,
                        const rhi_vertex_attribute>;

class rhi_deleter
{
public:
    rhi_deleter();
    rhi_deleter(rhi* rhi);

    void operator()(rhi_render_pass* render_pass);
    void operator()(rhi_shader* shader);
    void operator()(rhi_render_pipeline* render_pipeline);
    void operator()(rhi_compute_pipeline* compute_pipeline);
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

class render_device
{
public:
    render_device();
    ~render_device();

    static render_device& instance();

    void initialize(rhi* rhi);
    void reset();

    rhi_command* allocate_command();
    void execute(
        std::span<rhi_command*> commands,
        std::span<rhi_semaphore*> signal_semaphores,
        std::span<rhi_semaphore*> wait_semaphores,
        rhi_fence* fence);

    void begin_frame();
    void end_frame();

    rhi_fence* get_in_flight_fence();

    std::size_t get_frame_count() const noexcept;
    std::size_t get_frame_resource_count() const noexcept;
    std::size_t get_frame_resource_index() const noexcept;

    template <typename T>
    rhi_shader* get_shader()
    {
        static rhi_shader* shader = get_shader(get_shader_desc<T>());
        return shader;
    }

    const std::vector<std::string>& get_vertex_attributes(rhi_shader* shader) const
    {
        return m_shader_infos.at(shader).vertex_attribute;
    }

public:
    rhi_ptr<rhi_render_pass> create_render_pass(const rhi_render_pass_desc& desc);

    rhi_ptr<rhi_render_pipeline> create_pipeline(const rhi_render_pipeline_desc& desc);
    rhi_ptr<rhi_compute_pipeline> create_pipeline(const rhi_compute_pipeline_desc& desc);

    rhi_ptr<rhi_parameter> create_parameter(const rhi_parameter_desc& desc);
    rhi_ptr<rhi_framebuffer> create_framebuffer(const rhi_framebuffer_desc& desc);

    rhi_ptr<rhi_buffer> create_buffer(const rhi_buffer_desc& desc);
    rhi_ptr<rhi_sampler> create_sampler(const rhi_sampler_desc& desc);

    rhi_ptr<rhi_texture> create_texture(const rhi_texture_desc& desc);
    rhi_ptr<rhi_texture> create_texture_view(const rhi_texture_view_desc& desc);

    rhi_ptr<rhi_swapchain> create_swapchain(const rhi_swapchain_desc& desc);

    rhi_ptr<rhi_fence> create_fence(bool signaled);
    rhi_ptr<rhi_semaphore> create_semaphore();

    rhi_deleter& get_deleter() noexcept { return m_rhi_deleter; }

private:
    template <typename T>
    static consteval rhi_shader_desc get_shader_desc()
    {
        rhi_shader_desc desc = {};
        desc.stage = T::stage;
        desc.path = T::path.data();

        if constexpr (has_parameter<T>)
        {
            desc.parameters = T::parameters;
            desc.parameter_count = std::span(T::parameters).size();
        }

        if constexpr (T::stage == RHI_SHADER_STAGE_VERTEX && has_input<T>)
        {
            desc.vertex.vertex_attributes = T::inputs;
            desc.vertex.vertex_attribute_count = std::span(T::inputs).size();
        }

        return desc;
    }

    rhi_shader* get_shader(const rhi_shader_desc& desc);

    struct shader_info
    {
        std::vector<std::string> vertex_attribute;
    };
    std::unordered_map<rhi_shader*, shader_info> m_shader_infos;
    std::unordered_map<std::string, rhi_ptr<rhi_shader>> m_shader_cache;

    rhi* m_rhi{nullptr};
    rhi_deleter m_rhi_deleter;
};

struct shader_parameter
{
    consteval shader_parameter(std::initializer_list<rhi_parameter_binding> list) : desc{}
    {
        assert(list.size() <= rhi_constants::MAX_PARAMETER_BINDING_COUNT);

        for (std::size_t i = 0; i < list.size(); ++i)
            desc.bindings[i] = *(list.begin() + i);
        desc.binding_count = list.size();
    }

    consteval operator rhi_parameter_desc() const { return desc; }

    rhi_parameter_desc desc;
};

struct shader
{
    using parameter = shader_parameter;
    using parameter_slot = rhi_shader_desc::parameter_slot;

    static constexpr shader_parameter mesh = {
        {RHI_PARAMETER_UNIFORM, RHI_SHADER_STAGE_VERTEX, 64}
    };

    struct camera_data
    {
        float4x4 view;
        float4x4 projection;
        float4x4 view_projection;
        float3 position;
        std::uint32_t padding0;
    };

    static constexpr shader_parameter camera = {
        {RHI_PARAMETER_UNIFORM,
         RHI_SHADER_STAGE_VERTEX | RHI_SHADER_STAGE_FRAGMENT,
         sizeof(camera_data)                                   },
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT,    1}
    };

    static constexpr shader_parameter light = {
        {RHI_PARAMETER_UNIFORM, RHI_SHADER_STAGE_FRAGMENT, 528}
    };
};

template <typename T>
struct vertex_shader : public shader
{
    using input = rhi_vertex_attribute;

    static constexpr rhi_shader_stage_flag stage = RHI_SHADER_STAGE_VERTEX;

    static rhi_shader* get_rhi() { return render_device::instance().get_shader<T>(); }
};

template <typename T>
struct fragment_shader : public shader
{
    static constexpr rhi_shader_stage_flag stage = RHI_SHADER_STAGE_FRAGMENT;

    static rhi_shader* get_rhi() { return render_device::instance().get_shader<T>(); }
};

struct full_screen_vs : public vertex_shader<full_screen_vs>
{
    static constexpr std::string_view path = "engine/shaders/full_screen.vs";
};
} // namespace violet