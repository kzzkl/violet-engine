#pragma once

#include "graphics/render_device.hpp"
#include "math/math.hpp"
#include <cassert>

namespace violet
{
class pipeline_parameter_layout
{
public:
    constexpr pipeline_parameter_layout(std::initializer_list<rhi_parameter_binding> list)
    {
        assert(list.size() <= sizeof(m_desc.bindings) / sizeof(m_desc.bindings[0]));

        m_desc = {};
        for (auto& binding : list)
            m_desc.bindings[m_desc.binding_count++] = binding;
    }

    operator rhi_parameter_desc() const noexcept { return m_desc; }

private:
    rhi_parameter_desc m_desc{};
};

static constexpr pipeline_parameter_layout pipeline_parameter_mesh = {
    {RHI_PARAMETER_TYPE_UNIFORM, 64, RHI_SHADER_STAGE_FLAG_VERTEX}
};

struct pipeline_parameter_camera_data
{
    float4x4 view;
    float4x4 projection;
    float4x4 view_projection;
    float3 position;
    std::uint32_t pad0;
};

static constexpr pipeline_parameter_layout pipeline_parameter_camera = {
    {RHI_PARAMETER_TYPE_UNIFORM,
     sizeof(pipeline_parameter_camera_data),
     RHI_SHADER_STAGE_FLAG_VERTEX | RHI_SHADER_STAGE_FLAG_FRAGMENT         },
    {RHI_PARAMETER_TYPE_TEXTURE, 1,          RHI_SHADER_STAGE_FLAG_FRAGMENT}
};

static constexpr pipeline_parameter_layout pipeline_parameter_light = {
    {RHI_PARAMETER_TYPE_UNIFORM, 528, RHI_SHADER_STAGE_FLAG_FRAGMENT}
};
} // namespace violet