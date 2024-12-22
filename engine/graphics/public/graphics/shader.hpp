#pragma once

#include "graphics/render_interface.hpp"
#include "math/types.hpp"
#include <cassert>
#include <initializer_list>
#include <string_view>
#include <type_traits>

namespace violet
{
struct shader_parameter
{
    consteval shader_parameter(std::initializer_list<rhi_parameter_binding> list)
    {
        assert(list.size() <= rhi_constants::max_parameter_binding_count);

        for (std::size_t i = 0; i < list.size(); ++i)
        {
            bindings[i] = *(list.begin() + i);
        }
        binding_count = list.size();
    }

    consteval operator rhi_parameter_desc() const
    {
        return {bindings, binding_count};
    }

    rhi_parameter_binding bindings[rhi_constants::max_parameter_binding_count]{};
    std::size_t binding_count{};
};

struct shader
{
    using parameter = shader_parameter;

    struct parameter_layout
    {
        consteval parameter_layout(std::initializer_list<rhi_shader_desc::parameter_slot> list)
        {
            assert(list.size() <= rhi_constants::max_parameter_count);

            for (std::size_t i = 0; i < list.size(); ++i)
            {
                parameters[i] = *(list.begin() + i);
            }
            parameter_count = list.size();
        }

        rhi_shader_desc::parameter_slot parameters[rhi_constants::max_parameter_count]{};
        std::size_t parameter_count{};
    };

    struct draw_command
    {
        std::uint32_t index_count;
        std::uint32_t instance_count;
        std::uint32_t index_offset;
        std::int32_t vertex_offset;
        std::uint32_t instance;
    };

    struct mesh_data
    {
        mat4f model_matrix;
        vec3f aabb_min;
        std::uint32_t flags;
        vec3f aabb_max;
        std::uint32_t padding1;
    };

    struct instance_data
    {
        std::uint32_t mesh_index;
        std::uint32_t vertex_offset;
        std::uint32_t index_offset;
        std::uint32_t index_count;

        std::uint32_t group_index;
        std::uint32_t material_address;
        std::uint32_t flags;
        std::uint32_t padding0;
    };

    struct light_data
    {
        vec3f position;
        std::uint32_t type;
        vec3f direction;
        std::uint32_t shadow;
        vec3f color;
        std::uint32_t padding0;
    };

    struct scene_data
    {
        std::uint32_t mesh_buffer;
        std::uint32_t mesh_count;
        std::uint32_t instance_buffer;
        std::uint32_t instance_count;
        std::uint32_t group_buffer;
        std::uint32_t light_buffer;
        std::uint32_t light_count;
        std::uint32_t skybox;
        std::uint32_t irradiance;
        std::uint32_t prefilter;
        std::uint32_t brdf_lut;
        std::uint32_t material_buffer;
        std::uint32_t point_repeat_sampler;
        std::uint32_t point_clamp_sampler;
        std::uint32_t linear_repeat_sampler;
        std::uint32_t linear_clamp_sampler;
    };

    static constexpr parameter scene = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages =
                RHI_SHADER_STAGE_VERTEX | RHI_SHADER_STAGE_FRAGMENT | RHI_SHADER_STAGE_COMPUTE,
            .size = sizeof(scene_data),
        },
    };

    struct camera_data
    {
        mat4f view;
        mat4f projection;
        mat4f view_projection;
        mat4f view_projection_inv;
        vec3f position;
        std::uint32_t padding0;
    };

    static constexpr parameter camera = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages =
                RHI_SHADER_STAGE_VERTEX | RHI_SHADER_STAGE_FRAGMENT | RHI_SHADER_STAGE_COMPUTE,
            .size = sizeof(camera_data),
        },
    };

    static constexpr parameter bindless = {
        {
            .type = RHI_PARAMETER_BINDING_MUTABLE,
            .stages = RHI_SHADER_STAGE_ALL,
            .size = 0,
        },
        {
            .type = RHI_PARAMETER_BINDING_SAMPLER,
            .stages = RHI_SHADER_STAGE_FRAGMENT | RHI_SHADER_STAGE_COMPUTE,
            .size = 128,
        },
    };
};

template <typename T>
concept has_parameters =
    std::is_same_v<std::decay_t<decltype(T::parameters)>, shader::parameter_layout>;

struct shader_vs : public shader
{
    struct input_layout
    {
        consteval input_layout(std::initializer_list<rhi_vertex_attribute> list)
        {
            assert(list.size() <= rhi_constants::max_vertex_attribute_count);

            for (std::size_t i = 0; i < list.size(); ++i)
            {
                attributes[i] = *(list.begin() + i);
            }
            attribute_count = list.size();
        }

        rhi_vertex_attribute attributes[rhi_constants::max_vertex_attribute_count]{};
        std::size_t attribute_count;
    };

    static constexpr rhi_shader_stage_flag stage = RHI_SHADER_STAGE_VERTEX;
};

template <typename T>
concept has_inputs = std::is_same_v<std::decay_t<decltype(T::inputs)>, shader_vs::input_layout>;

struct shader_fs : public shader
{
    static constexpr rhi_shader_stage_flag stage = RHI_SHADER_STAGE_FRAGMENT;
};

struct shader_cs : public shader
{
    static constexpr rhi_shader_stage_flag stage = RHI_SHADER_STAGE_COMPUTE;
};

struct fullscreen_vs : public shader_vs
{
    static constexpr std::string_view path = "assets/shaders/fullscreen.hlsl";
};

struct mesh_vs : public shader_vs
{
    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, scene},
        {2, camera},
    };
};

struct mesh_fs : public shader_fs
{
};

struct skinning_cs : public shader_cs
{
    struct skinning_data
    {
        std::uint32_t skeleton;
        std::uint32_t buffers[11];
    };

    static constexpr parameter skinning = {
        {RHI_PARAMETER_BINDING_CONSTANT, RHI_SHADER_STAGE_COMPUTE, sizeof(skinning_data)},
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, skinning},
    };
};
} // namespace violet