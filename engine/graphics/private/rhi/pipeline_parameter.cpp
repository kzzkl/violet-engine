#include "graphics/rhi/pipeline_parameter.hpp"
#include "core/context/engine.hpp"
#include "graphics/graphics.hpp"

namespace violet
{
pipeline_parameter::pipeline_parameter(const pipeline_parameter_desc& desc)
{
    auto& engine_graphics = engine::get_module<graphics>();
    m_interface.reset(engine_graphics.get_rhi()->make_pipeline_parameter(desc));
}

void* pipeline_parameter::get_field_pointer(std::size_t index)
{
    return m_interface->get_constant_buffer_pointer(index);
}
} // namespace violet