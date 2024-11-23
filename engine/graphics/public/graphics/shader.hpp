#pragma once

#include "graphics/render_interface.hpp"
#include "math/math.hpp"
#include <cassert>
#include <string>

namespace violet
{
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

    consteval operator rhi_parameter_desc() const
    {
        return {bindings, binding_count};
    }

    rhi_parameter_binding bindings[rhi_constants::MAX_PARAMETER_BINDING_COUNT];
    std::size_t binding_count;
};

struct shader
{
    using parameter = shader_parameter;

    struct parameter_layout
    {
        consteval parameter_layout(std::initializer_list<rhi_shader_desc::parameter_slot> list)
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

    struct draw_command
    {
        std::uint32_t index_count;
        std::uint32_t instance_count;
        std::uint32_t index_offset;
        std::int32_t vertex_offset;
        std::uint32_t instance;
    };

    static constexpr parameter global = {
        {RHI_PARAMETER_STORAGE,
         RHI_SHADER_STAGE_VERTEX | RHI_SHADER_STAGE_FRAGMENT | RHI_SHADER_STAGE_COMPUTE,
         1},
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
        light_data lights[32];
        std::uint32_t light_count;
        std::uint32_t mesh_count;
        std::uint32_t instance_count;
        std::uint32_t padding0;
    };

    static constexpr parameter scene = {
        {RHI_PARAMETER_UNIFORM,
         RHI_SHADER_STAGE_VERTEX | RHI_SHADER_STAGE_FRAGMENT | RHI_SHADER_STAGE_COMPUTE,
         sizeof(scene_data)},
        {RHI_PARAMETER_STORAGE,
         RHI_SHADER_STAGE_VERTEX | RHI_SHADER_STAGE_FRAGMENT | RHI_SHADER_STAGE_COMPUTE,
         1}, // Mesh Buffer
        {RHI_PARAMETER_STORAGE,
         RHI_SHADER_STAGE_VERTEX | RHI_SHADER_STAGE_FRAGMENT | RHI_SHADER_STAGE_COMPUTE,
         1}, // Instance Buffer
    };

    struct camera_data
    {
        mat4f view;
        mat4f projection;
        mat4f view_projection;
        vec3f position;
        std::uint32_t padding0;
    };

    static constexpr parameter camera = {
        {RHI_PARAMETER_UNIFORM,
         RHI_SHADER_STAGE_VERTEX | RHI_SHADER_STAGE_FRAGMENT | RHI_SHADER_STAGE_COMPUTE,
         sizeof(camera_data)},
    };

    static constexpr parameter gbuffer = {
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1}, // Albedo
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1}, // Depth Buffer
    };
};

struct shader_empty_option
{
};

template <typename T>
concept has_parameters =
    std::is_same_v<std::decay_t<decltype(T::parameters)>, shader::parameter_layout>;

struct shader_vs : public shader
{
    struct input_layout
    {
        consteval input_layout(std::initializer_list<rhi_vertex_attribute> list)
            : attributes{}
        {
            assert(list.size() <= rhi_constants::MAX_VERTEX_ATTRIBUTE_COUNT);

            for (std::size_t i = 0; i < list.size(); ++i)
                attributes[i] = *(list.begin() + i);
            attribute_count = list.size();
        }

        rhi_vertex_attribute attributes[rhi_constants::MAX_VERTEX_ATTRIBUTE_COUNT];
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
    static constexpr std::string_view path = "assets/shaders/source/fullscreen.hlsl";
};

struct mesh_vs : public shader_vs
{
    static constexpr parameter_layout parameters = {
        {0, global},
        {1, scene},
        {2, camera},
    };
};

struct mesh_fs : public shader_fs
{
};
} // namespace violet