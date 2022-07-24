#pragma once

#include "graphics/pipeline_parameter.hpp"
#include "graphics_interface.hpp"
#include <memory>

namespace ash::graphics
{
class shadow_map_pipeline_parameter : public pipeline_parameter
{
public:
    shadow_map_pipeline_parameter();

    void light_view_projection(const math::float4x4& view_projection);

    static std::vector<pipeline_parameter_pair> layout();

private:
    struct constant_data
    {
        math::float4x4 light_view_projection;
    };
};

class shadow_map
{
public:
    shadow_map(std::size_t resolution);

    void parameter(shadow_map_pipeline_parameter* parameter) noexcept { m_parameter = parameter; }
    pipeline_parameter_interface* parameter() const noexcept { return m_parameter->interface(); }

    resource_interface* depth_buffer() const noexcept { return m_shadow_map.get(); }

    std::unique_ptr<resource_interface> m_test;
private:
    std::unique_ptr<resource_interface> m_shadow_map;
    shadow_map_pipeline_parameter* m_parameter;
};
} // namespace ash::graphics