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
    render_parameter(pipeline_parameter_interface* parameter) : m_parameter(parameter) {}

    template <typename T>
    void set(std::size_t index, const T& value) noexcept
    {
        m_parameter->set(index, value);
    }

    void set(std::size_t index, const math::float4x4& value) { m_parameter->set(index, value); }

    void set(std::size_t index, const math::float4x4* value, std::size_t size)
    {
        m_parameter->set(index, value, size);
    }

    pipeline_parameter_interface* parameter() const { return m_parameter.get(); }

private:
    std::unique_ptr<pipeline_parameter_interface> m_parameter;
};
} // namespace ash::graphics