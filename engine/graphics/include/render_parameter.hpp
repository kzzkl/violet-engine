#pragma once

#include "graphics_interface.hpp"
#include "math.hpp"
#include "texture.hpp"
#include "type_trait.hpp"
#include <memory>
#include <vector>

namespace ash::graphics
{
class render_parameter
{
public:
    render_parameter(pipeline_parameter* parameter) : m_parameter(parameter) {}

    template <typename T>
    void set(std::size_t index, const T& value) noexcept
    {
        m_parameter->set(index, value);
    }

    void set(std::size_t index, const math::float4x4& value, bool row_matrix = true)
    {
        m_parameter->set(index, value, row_matrix);
    }

    void set(
        std::size_t index,
        const math::float4x4* value,
        std::size_t size,
        bool row_matrix = true)
    {
        m_parameter->set(index, value, size, row_matrix);
    }

    pipeline_parameter* parameter() const { return m_parameter.get(); }

private:
    std::unique_ptr<pipeline_parameter> m_parameter;
};

/*struct render_pass_data
{
    math::float4 camera_position;
    math::float4 camera_direction;

    math::float4x4 camera_view;
    math::float4x4 camera_projection;
    math::float4x4 camera_view_projection;
};
using render_parameter_pass = render_parameter<multiple<render_pass_data>>;

struct render_object_data
{
    math::float4x4 to_world;
};
using render_parameter_object = render_parameter<multiple<render_object_data>>;*/
} // namespace ash::graphics