#include "graphics/material.hpp"
#include "graphics/pipeline_parameter.hpp"

namespace violet
{
material::material()
{
}

void material::add_pass(const rdg_render_pipeline& pipeline, const rhi_parameter_desc& parameter)
{
    m_passes.push_back(pipeline);

    rhi_parameter_desc* parameters = m_passes.back().parameters;
    parameters[0] = pipeline_parameter_mesh;
    parameters[1] = parameter;
    parameters[2] = pipeline_parameter_camera;
    parameters[3] = pipeline_parameter_light;
    m_passes.back().parameter_count = 4;

    m_parameters.push_back(render_device::instance().create_parameter(parameter));
}
} // namespace violet