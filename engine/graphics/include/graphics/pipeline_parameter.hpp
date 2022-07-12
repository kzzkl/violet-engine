#pragma once

#include "graphics_interface.hpp"
#include <memory>
#include <string_view>
#include <vector>

namespace ash::graphics
{
class pipeline_parameter
{
public:
    pipeline_parameter(std::string_view layout_name);
    virtual ~pipeline_parameter() = default;

    pipeline_parameter_interface* interface() const noexcept { return m_interface.get(); }

protected:
    template <typename T>
    T& field(std::size_t index)
    {
        return *static_cast<T*>(field_pointer(index));
    }

    void* field_pointer(std::size_t index);

private:
    std::unique_ptr<pipeline_parameter_interface> m_interface;
};
} // namespace ash::graphics