#include "graphics/render_graph/material.hpp"

namespace violet
{
material_layout::material_layout(std::string_view name, rhi_context* rhi) : render_node(name, rhi)
{
}

void material_layout::add_pipeline(render_pipeline& pipeline)
{
    m_pipelines.push_back(&pipeline);
}

material* material_layout::add_material(std::string_view name)
{
    m_materials[name.data()] = std::make_unique<material>(this);
    return m_materials[name.data()].get();
}

material::material(material_layout* layout) : m_layout(layout)
{
    for (render_pipeline* pipeline : layout->get_pipelines())
    {
        rhi_pipeline_parameter_layout* parameter_layout =
            pipeline->get_parameter_layout(RENDER_PIPELINE_PARAMETER_TYPE_MATERIAL);

        if (parameter_layout != nullptr)
            m_parameters.push_back(layout->get_rhi()->create_pipeline_parameter(parameter_layout));
        else
            m_parameters.push_back(nullptr);
    }
}

material::~material()
{
    for (rhi_pipeline_parameter* parameter : m_parameters)
    {
        if (parameter != nullptr)
            m_layout->get_rhi()->destroy_pipeline_parameter(parameter);
    }
}

void material::set(std::string_view name, rhi_resource* texture, rhi_sampler* sampler)
{
    auto& field = m_layout->get_field(name);
    m_parameters[field.pipeline_index]->set(field.field_index, texture, sampler);
}
} // namespace violet