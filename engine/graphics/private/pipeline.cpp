#include "graphics/pipeline.hpp"
#include "common/assert.hpp"

namespace violet::graphics
{
pipeline_parameter::pipeline_parameter(const pipeline_parameter_desc& desc)
{
    m_interface = rhi::make_pipeline_parameter(desc);
}

void* pipeline_parameter::field_pointer(std::size_t index)
{
    return m_interface->constant_buffer_pointer(index);
}
} // namespace violet::graphics