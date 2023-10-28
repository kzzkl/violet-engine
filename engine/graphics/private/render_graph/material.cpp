#include "graphics/render_graph/material.hpp"

namespace violet
{
material_layout::material_layout(graphics_context* context) : render_node(context)
{
}

void material_layout::add_pipeline(render_pipeline* pipeline)
{
    m_pipelines.push_back(pipeline);
}

material* material_layout::add_material(std::string_view name)
{
    m_materials[name.data()] = std::make_unique<material>(this);
    return m_materials[name.data()].get();
}

material::material(material_layout* layout) : m_layout(layout)
{
    rhi_renderer* rhi = layout->get_context()->get_rhi();
    for (render_pipeline* pipeline : layout->m_pipelines)
    {
        rhi_parameter_layout* parameter_layout =
            pipeline->get_parameter_layout(RENDER_PIPELINE_PARAMETER_TYPE_MATERIAL);

        if (parameter_layout != nullptr)
            m_parameters.push_back(rhi->create_parameter(parameter_layout));
        else
            m_parameters.push_back(nullptr);
    }
}

material::~material()
{
    rhi_renderer* rhi = m_layout->get_context()->get_rhi();
    for (rhi_parameter* parameter : m_parameters)
    {
        if (parameter != nullptr)
            rhi->destroy_parameter(parameter);
    }
}

void material::set(std::string_view name, rhi_resource* texture, rhi_sampler* sampler)
{
    auto& field = m_layout->get_field(name);
    m_parameters[field.pipeline_index]->set(field.field_index, texture, sampler);
}
} // namespace violet