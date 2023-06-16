#include "graphics/pipeline_parameter.hpp"
#include "core/context/engine.hpp"
#include "graphics/graphics_module.hpp"

namespace violet
{
pipeline_parameter::pipeline_parameter(const rhi_pipeline_parameter_desc& desc)
{
    auto& graphics = engine::get_module<graphics_module>();
    m_interface.reset(graphics.get_rhi()->make_pipeline_parameter(desc));
}

void pipeline_parameter::set(std::size_t index, const void* data, size_t size)
{
    m_interface->set(index, data, size);
}

void pipeline_parameter::set(std::size_t index, rhi_resource* texture)
{
    m_interface->set(index, texture);
}
} // namespace violet