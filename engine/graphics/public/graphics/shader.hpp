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
        assert(list.size() <= rhi_constants::max_parameter_bindings);

        for (std::size_t i = 0; i < list.size(); ++i)
        {
            bindings[i] = *(list.begin() + i);
        }
        binding_count = static_cast<std::uint32_t>(list.size());
    }

    consteval operator rhi_parameter_desc() const
    {
        return {bindings, binding_count};
    }

    rhi_parameter_binding bindings[rhi_constants::max_parameter_bindings]{};
    std::uint32_t binding_count{};
};

struct shader
{
    using parameter = shader_parameter;

    struct parameter_layout
    {
        consteval parameter_layout(std::initializer_list<rhi_shader_desc::parameter_slot> list)
        {
            assert(list.size() <= rhi_constants::max_parameters);

            for (std::size_t i = 0; i < list.size(); ++i)
            {
                parameters[i] = *(list.begin() + i);
            }
            parameter_count = static_cast<std::uint32_t>(list.size());
        }

        rhi_shader_desc::parameter_slot parameters[rhi_constants::max_parameters]{};
        std::uint32_t parameter_count{};
    };

    struct draw_command
    {
        std::uint32_t index_count;
        std::uint32_t instance_count;
        std::uint32_t index_offset;
        std::int32_t vertex_offset;
        std::uint32_t instance;
    };

    struct geometry_data
    {
        vec4f bounding_sphere;
        std::uint32_t position_address;
        std::uint32_t normal_address;
        std::uint32_t tangent_address;
        std::uint32_t texcoord_address;
        std::uint32_t custom0_address;
        std::uint32_t custom1_address;
        std::uint32_t custom2_address;
        std::uint32_t custom3_address;
        std::uint32_t index_offset;
        std::uint32_t index_count;
        std::uint32_t cluster_root;
        std::uint32_t padding0;
    };

    struct cluster_data
    {
        vec4f bounding_sphere;
        vec4f lod_bounds;

        float lod_error;
        std::uint32_t index_offset;
        std::uint32_t index_count;
        std::uint32_t padding0;
    };

    struct cluster_node_data
    {
        vec4f bounding_sphere;
        vec4f lod_bounds;

        float min_lod_error;
        float max_parent_lod_error;

        std::uint32_t is_leaf;
        std::uint32_t child_offset;
        std::uint32_t child_count;

        std::uint32_t padding0;
        std::uint32_t padding1;
        std::uint32_t padding2;
    };

    struct mesh_data
    {
        mat4f matrix_m;
    };

    struct instance_data
    {
        std::uint32_t mesh_index;
        std::uint32_t geometry_index;
        std::uint32_t batch_index;
        std::uint32_t material_address;
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
        std::uint32_t light_buffer;
        std::uint32_t light_count;
        std::uint32_t batch_buffer;
        std::uint32_t material_buffer;
        std::uint32_t geometry_buffer;
        std::uint32_t vertex_buffer;
        std::uint32_t index_buffer;
        std::uint32_t skybox;
        std::uint32_t irradiance;
        std::uint32_t prefilter;
        std::uint32_t padding0;
        std::uint32_t padding1;
    };

    static constexpr parameter scene = {
        {
            .type = RHI_PARAMETER_BINDING_UNIFORM,
            .stages = RHI_SHADER_STAGE_ALL,
            .size = sizeof(scene_data),
        },
    };

    struct camera_data
    {
        mat4f matrix_v;
        mat4f matrix_p;
        mat4f matrix_p_inv;
        mat4f matrix_vp;
        mat4f matrix_vp_inv;
        mat4f matrix_vp_no_jitter;

        mat4f prev_matrix_vp;
        mat4f prev_matrix_vp_no_jitter;

        vec3f position;
        float fov;

        float near;
        float far;
        float width;
        float height;

        vec2f jitter;
        std::uint32_t padding0;
        std::uint32_t padding1;
    };

    static constexpr parameter camera = {
        {
            .type = RHI_PARAMETER_BINDING_UNIFORM,
            .stages = RHI_SHADER_STAGE_ALL,
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
            .size = 0,
        },
    };
};

template <typename T>
concept has_parameters =
    std::is_same_v<std::decay_t<decltype(T::parameters)>, shader::parameter_layout>;

template <typename T>
concept has_constant = requires { typename T::constant_data; };

struct shader_vs : public shader
{
    struct input_layout
    {
        consteval input_layout(std::initializer_list<rhi_vertex_attribute> list)
        {
            assert(list.size() <= rhi_constants::max_vertex_attributes);

            for (std::size_t i = 0; i < list.size(); ++i)
            {
                attributes[i] = *(list.begin() + i);
            }
            attribute_count = list.size();
        }

        rhi_vertex_attribute attributes[rhi_constants::max_vertex_attributes]{};
        std::size_t attribute_count;
    };

    static constexpr rhi_shader_stage_flag stage = RHI_SHADER_STAGE_VERTEX;
};

template <typename T>
concept has_inputs = std::is_same_v<std::decay_t<decltype(T::inputs)>, shader_vs::input_layout>;

struct shader_gs : public shader
{
    static constexpr rhi_shader_stage_flag stage = RHI_SHADER_STAGE_GEOMETRY;
};

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
    struct constant_data
    {
        std::uint32_t position_input_address;
        std::uint32_t normal_input_address;
        std::uint32_t tangent_input_address;
        std::uint32_t position_output_address;
        std::uint32_t normal_output_address;
        std::uint32_t tangent_output_address;
        std::uint32_t vertex_buffer;
        std::uint32_t skeleton;
        std::uint32_t additional[4];
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
    };
};
} // namespace violet