#pragma once

#include "graphics/light.hpp"
#include "graphics/pipeline.hpp"
#include "graphics_interface.hpp"
#include <memory>

namespace violet::graphics
{
class shadow_map_pipeline_parameter : public pipeline_parameter
{
public:
    struct constant_data
    {
        math::float4x4 light_view_projection;
    };

    static constexpr pipeline_parameter_desc layout = {
        .parameters = {PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER, sizeof(constant_data)},
        .parameter_count = 1
    };

public:
    shadow_map_pipeline_parameter();

    void light_view_projection(const math::float4x4& view_projection);
};

class shadow_pipeline;
class shadow_map
{
public:
    shadow_map(std::uint32_t resolution);

    void light_view_projection(const math::float4x4& view_projection);

    pipeline_parameter_interface* parameter() const noexcept { return m_parameter->interface(); }
    resource_interface* depth_buffer() const noexcept { return m_shadow_map.get(); }

private:
    std::unique_ptr<resource_interface> m_shadow_map;
    std::unique_ptr<shadow_map_pipeline_parameter> m_parameter;
};
} // namespace violet::graphics