#pragma once

#include "graphics/pipeline_parameter.hpp"
#include "graphics/shadow_map.hpp"

namespace ash::graphics
{
struct point_light
{
};

class directional_light
{
public:
    directional_light();

    void color(const math::float3& color) noexcept { m_color = color; }
    const math::float3& color() const noexcept { return m_color; }

    shadow_map* shadow() const noexcept { return m_shadow_map.get(); }

private:
    math::float3 m_color;

    std::unique_ptr<shadow_map> m_shadow_map;
};

class light_pipeline_parameter : public pipeline_parameter
{
public:
    static constexpr std::size_t MAX_DIRECTIONAL_LIGHT_COUNT = 4;
    static constexpr std::size_t MAX_POINT_LIGHT_COUNT = 20;

public:
    light_pipeline_parameter();

    void directional_light(
        std::size_t index,
        const math::float3& color,
        const math::float3& direction);
    void directional_light_count(std::size_t count);

    static std::vector<pipeline_parameter_pair> layout();

private:
    struct directional_light_data
    {
        math::float3 direction;
        float _padding_0;
        math::float3 color;
        float _padding_1;
    };

    struct constant_data
    {
        directional_light_data directional_lights[MAX_DIRECTIONAL_LIGHT_COUNT];
        std::uint32_t directional_light_count;
    };
};
} // namespace ash::graphics