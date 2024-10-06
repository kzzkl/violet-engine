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

    struct mesh_data
    {
        float4x4 model_matrix;
        float4x4 normal_matrix;
    };

    static constexpr parameter mesh = {
        {RHI_PARAMETER_UNIFORM,
         RHI_SHADER_STAGE_VERTEX | RHI_SHADER_STAGE_FRAGMENT,
         sizeof(mesh_data)}};

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
         sizeof(camera_data)},
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1}};

    struct directional_light
    {
        float3 direction;
        bool shadow;
        float3 color;
        std::uint32_t padding;
    };

    struct light_data
    {
        directional_light directional_lights[16];
        std::uint32_t directional_light_count;

        uint3 padding;
    };

    static constexpr parameter light = {{RHI_PARAMETER_UNIFORM, RHI_SHADER_STAGE_FRAGMENT, 528}};
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

struct fullscreen_vs : public shader_vs
{
    static constexpr std::string_view path = "assets/shaders/source/fullscreen.hlsl";
};
} // namespace violet