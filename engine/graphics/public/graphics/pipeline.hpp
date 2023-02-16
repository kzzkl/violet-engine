#pragma once

#include "graphics/rhi.hpp"
#include <vector>

namespace violet::graphics
{
class pipeline_parameter
{
public:
    pipeline_parameter(const pipeline_parameter_desc& desc);
    virtual ~pipeline_parameter() {}

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
} // namespace violet::graphics