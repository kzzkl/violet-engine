#pragma once

#include "interface/graphics_interface.hpp"
#include <memory>

namespace violet
{
class pipeline_parameter
{
public:
    pipeline_parameter(const pipeline_parameter_desc& desc);
    virtual ~pipeline_parameter() {}

    pipeline_parameter_interface* get_interface() const noexcept { return m_interface.get(); }

    template <typename T>
    T& get_field(std::size_t index)
    {
        return *static_cast<T*>(get_field_pointer(index));
    }

    void* get_field_pointer(std::size_t index);

private:
    std::unique_ptr<pipeline_parameter_interface> m_interface;
};
} // namespace violet