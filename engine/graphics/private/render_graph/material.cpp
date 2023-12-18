#include "graphics/render_graph/material.hpp"

namespace violet
{
material_layout::material_layout()
{
}

void material_layout::add_pipeline(render_pipeline* pipeline)
{
    m_pipelines.push_back(pipeline);
}

material* material_layout::add_material(std::string_view name, renderer* renderer)
{
    m_materials[name.data()] = std::make_unique<material>(this, renderer);
    return m_materials.at(name.data()).get();
}

material* material_layout::get_material(std::string_view name) const
{
    return m_materials.at(name.data()).get();
}

material::material(material_layout* layout, renderer* renderer) : m_layout(layout)
{
    for (render_pipeline* pipeline : layout->m_pipelines)
    {
        rhi_parameter_layout* parameter_layout =
            pipeline->get_parameter_layout(RENDER_PIPELINE_PARAMETER_TYPE_MATERIAL);

        if (parameter_layout != nullptr)
            m_parameters.push_back(renderer->create_parameter(parameter_layout));
        else
            m_parameters.push_back(nullptr);
    }
}

material::~material()
{
}

void material::set(std::string_view name, rhi_resource* storage_buffer)
{
    auto& field = m_layout->get_field(name);
    m_parameters[field.pipeline_index]->set_storage(field.field_index, storage_buffer);
}

void material::set(std::string_view name, rhi_resource* texture, rhi_sampler* sampler)
{
    auto& field = m_layout->get_field(name);
    m_parameters[field.pipeline_index]->set_texture(field.field_index, texture, sampler);
}
} // namespace violet