#include "graphics/material.hpp"

namespace violet
{
material::material() {}

void material::add_pass(const rdg_render_pipeline& pipeline, const rhi_parameter_desc& parameter)
{
    m_passes.push_back(
        {.pipeline = pipeline,
         .parameter = parameter.bindings[0].size != 0 ?
                          render_device::instance().create_parameter(parameter) :
                          nullptr});
}
} // namespace violet