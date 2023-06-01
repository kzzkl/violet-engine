#include "graphics/render_graph/pipeline_parameter.hpp"
#include "core/context/engine.hpp"
#include "graphics/graphics_module.hpp"

namespace violet
{
pipeline_parameter::pipeline_parameter(const pipeline_parameter_desc& desc)
{
    auto& graphics = engine::get_module<graphics_module>();
    m_interface.reset(graphics.get_rhi()->make_pipeline_parameter(desc));
}

void* pipeline_parameter::get_field_pointer(std::size_t index)
{
    return m_interface->get_constant_buffer_pointer(index);
}
} // namespace violet