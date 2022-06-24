#include "graphics/pipeline_parameter.hpp"
#include "assert.hpp"
#include "graphics/rhi.hpp"

namespace ash::graphics
{
pipeline_parameter::pipeline_parameter(std::string_view layout_name)
{
    auto layout_interface = rhi::find_pipeline_parameter_layout(layout_name);
    ASH_ASSERT(layout_interface);
    m_interface = rhi::make_pipeline_parameter(layout_interface);
}

void* pipeline_parameter::field_pointer(std::size_t index)
{
    return m_interface->constant_buffer_pointer(index);
}
} // namespace ash::graphics