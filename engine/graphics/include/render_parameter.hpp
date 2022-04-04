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
} // namespace ash::graphics