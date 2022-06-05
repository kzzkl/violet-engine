#pragma once

#include "graphics_interface.hpp"
#include <memory>
#include <vector>

namespace ash::graphics
{
class pipeline_parameter
{
public:
    pipeline_parameter(pipeline_parameter_interface* interface) : m_interface(interface) {}

    template <typename T>
    void set(std::size_t index, const T& value) noexcept
    {
        m_interface->set(index, value);
    }

    void set(std::size_t index, const math::float4x4& value) { m_interface->set(index, value); }

    void set(std::size_t index, const math::float4x4* value, std::size_t size)
    {
        m_interface->set(index, value, size);
    }

    void reset() { m_interface->reset(); }

    pipeline_parameter_interface* interface() const { return m_interface.get(); }

private:
    std::unique_ptr<pipeline_parameter_interface> m_interface;
};
} // namespace ash::graphics