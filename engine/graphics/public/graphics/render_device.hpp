#pragma once

#include "common/hash.hpp"
#include "graphics/render_interface.hpp"
#include "math/math.hpp"
#include <cassert>
#include <memory>
#include <span>
#include <string>
#include <tuple>
#include <unordered_map>

namespace violet
{
template <typename... T>
class shader_options
{
public:
    template <typename U>
    void set(U value)
    {
        std::get<U>(m_options) = value;
    }

    template <typename U>
    U get() const
    {
        return std::get<U>(m_options);
    }

private:
    std::tuple<T...> m_options;
};

struct shader_parameter
{
    consteval shader_parameter(std::initializer_list<rhi_parameter_binding> list)
        : bindings{},
          binding_count(0)
    {
        assert(list.size() <= rhi_constants::MAX_PARAMETER_BINDING_COUNT);

        for (std::size_t i = 0; i < list.size(); ++i)
            bindings[i] = *(list.begin() + i);
        binding_count = list.size();
    }

    consteval operator rhi_parameter_desc() const { return {bindings, binding_count}; }

    rhi_parameter_binding bindings[rhi_constants::MAX_PARAMETER_BINDING_COUNT];
    std::size_t binding_count;
};

struct shader
{
    template <typename... T>
    using options = shader_options<T...>;
    using parameter = shader_parameter;

    struct parameter_slots
    {
        consteval parameter_slots(std::initializer_list<rhi_shader_desc::parameter_slot> list)
            : parameters{},
              parameter_count(0)
        {
            assert(list.size() <= rhi_constants::MAX_PARAMETER_COUNT);

            for (std::size_t i = 0; i < list.size(); ++i)
                parameters[i] = *(list.begin() + i);
            parameter_count = list.size();
        }

        rhi_shader_desc::parameter_slot parameters[rhi_constants::MAX_PARAMETER_COUNT];
        std::size_t parameter_count;
    };

    struct mesh_data
    {
        float4x4 model_matrix;
        float4x4 normal_matrix;
    };

    static constexpr parameter mesh = {
        {RHI_PARAMETER_UNIFORM,
         RHI_SHADER_STAGE_VERTEX | RHI_SHADER_STAGE_FRAGMENT,
         sizeof(mesh_data)}
    };

    struct camera_data
    {
        float4x4 view;
        float4x4 projection;
        float4x4 view_projection;
        float3 position;
        std::uint32_t padding0;
    };

    static constexpr parameter camera = {
        {RHI_PARAMETER_UNIFORM,
         RHI_SHADER_STAGE_VERTEX | RHI_SHADER_STAGE_FRAGMENT,
         sizeof(camera_data)                                   },
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT,    1}
    };

    static constexpr parameter light = {
        {RHI_PARAMETER_UNIFORM, RHI_SHADER_STAGE_FRAGMENT, 528}
    };
};

template <typename T>
concept has_options = requires { typename T::options; };

struct shader_empty_option
{
};

template <typename T>
concept has_parameter = (has_options<T> && requires(T t) {
                            {
                                T::get_parameters(T::options())
                            } -> std::same_as<shader::parameter_slots>;
                        }) || (!has_options<T> && requires(T t) {
                            { T::get_parameters() } -> std::same_as<shader::parameter_slots>;
                        });

struct vertex_shader : public shader
{
    struct input_slots
    {
        consteval input_slots(std::initializer_list<rhi_vertex_attribute> list)
            : vertex_attributes{}
        {
            assert(list.size() <= rhi_constants::MAX_VERTEX_ATTRIBUTE_COUNT);

            for (std::size_t i = 0; i < list.size(); ++i)
                vertex_attributes[i] = *(list.begin() + i);
            vertex_attribute_count = list.size();
        }

        rhi_vertex_attribute vertex_attributes[rhi_constants::MAX_VERTEX_ATTRIBUTE_COUNT];
        std::size_t vertex_attribute_count;
    };

    static constexpr rhi_shader_stage_flag stage = RHI_SHADER_STAGE_VERTEX;
};

template <typename T>
concept has_input = (has_options<T> && requires(T t) {
                        { T::get_inputs(T::options()) } -> std::same_as<vertex_shader::input_slots>;
                    }) || (!has_options<T> && requires(T t) {
                        { T::get_inputs() } -> std::same_as<vertex_shader::input_slots>;
                    });

struct fragment_shader : public shader
{
    static constexpr rhi_shader_stage_flag stage = RHI_SHADER_STAGE_FRAGMENT;
};

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

    template <typename T, typename Option = shader_empty_option>
    rhi_shader* get_shader(const Option& options = {})
    {
        static std::unordered_map<std::uint64_t, rhi_shader*> shader_maps;

        std::uint64_t hash = hash::city_hash_64(&options, sizeof(Option));
        auto iter = shader_maps.find(hash);
        if (iter != shader_maps.end())
        {
            return iter->second;
        }
        else
        {
            rhi_shader_desc desc = {};
            desc.stage = T::stage;

            if constexpr (has_options<T>)
                desc.path = T::get_path(options).data();
            else
                desc.path = T::get_path().data();

            shader::parameter_slots parameters = {};
            vertex_shader::input_slots inputs = {};

            if constexpr (has_parameter<T>)
            {
                if constexpr (has_options<T>)
                    parameters = T::get_parameters(options);
                else
                    parameters = T::get_parameters();
                desc.parameters = parameters.parameters;
                desc.parameter_count = parameters.parameter_count;
            }

            if constexpr (T::stage == RHI_SHADER_STAGE_VERTEX && has_input<T>)
            {
                if constexpr (has_options<T>)
                    inputs = T::get_inputs(options);
                else
                    inputs = T::get_inputs();

                desc.vertex.vertex_attributes = inputs.vertex_attributes;
                desc.vertex.vertex_attribute_count = inputs.vertex_attribute_count;
            }

            rhi_shader* shader = get_shader(desc);
            shader_maps[hash] = shader;
            return shader;
        }
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

struct full_screen_vs : public vertex_shader
{
    static constexpr std::string_view get_path() { return "engine/shaders/full_screen.vs"; }
};
} // namespace violet