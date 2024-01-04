#include "graphics/render_graph/compute_pass.hpp"

namespace violet
{
compute_pipeline::compute_pipeline() : m_interface(nullptr)
{
}

compute_pipeline::~compute_pipeline()
{
}

void compute_pipeline::set_shader(std::string_view compute)
{
    m_compute_shader = compute;
}

void compute_pipeline::set_parameter_layouts(
    const std::vector<rhi_parameter_layout*>& parameter_layouts)
{
    m_parameter_layouts = parameter_layouts;
}

rhi_parameter_layout* compute_pipeline::get_parameter_layout(std::size_t index) const noexcept
{
    return m_parameter_layouts[index];
}

bool compute_pipeline::compile(compile_context& context)
{
    rhi_compute_pipeline_desc desc = {};

    std::vector<rhi_parameter_layout*> parameter_layouts;
    for (rhi_parameter_layout* parameter : m_parameter_layouts)
        parameter_layouts.push_back(parameter);

    desc.parameters = parameter_layouts.data();
    desc.parameter_count = parameter_layouts.size();
    desc.compute_shader = m_compute_shader.c_str();

    m_interface = context.renderer->create_compute_pipeline(desc);
    return m_interface != nullptr;
}

void compute_pipeline::add_dispatch(
    std::size_t x,
    std::size_t y,
    std::size_t z,
    const std::vector<rhi_parameter*>& parameters)
{
    m_compute_data.push_back({x, y, z, parameters});
}

compute_pass::compute_pass(renderer* renderer, setup_context& context) : pass(renderer, context)
{
}

compute_pass::~compute_pass()
{
}

compute_pipeline* compute_pass::get_pipeline(std::string_view name) const
{
    return m_pipeline_map.at(name.data());
}

bool compute_pass::compile(compile_context& context)
{
    for (auto& pipeline : m_pipelines)
    {
        if (!pipeline->compile(context))
            return false;
    }
    return true;
}

void compute_pass::execute(execute_context& context)
{
}
} // namespace violet