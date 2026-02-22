#include "graphics/material.hpp"
#include "graphics/material_manager.hpp"

namespace violet
{
material::material() noexcept
{
    auto* material_manager = render_device::instance().get_material_manager();
    m_material_id = material_manager->add_material(this);
}

material::~material()
{
    auto* material_manager = render_device::instance().get_material_manager();
    material_manager->remove_material(m_material_id);

    if (m_resolve_pipeline != 0)
    {
        material_manager->remove_material_resolve_pipeline(m_resolve_pipeline);
    }
}

void material::update()
{
    if (!m_dirty)
    {
        return;
    }

    auto* material_manager = render_device::instance().get_material_manager();

    auto [data, size] = get_constant_data();
    material_manager->update_constant(m_material_id, data, size);

    m_dirty = false;
}

void material::set_pipeline_impl(const rdg_raster_pipeline& pipeline)
{
    assert(m_pipeline.vertex_shader == nullptr);

    m_pipeline = pipeline;
}

void material::set_pipeline_impl(
    const rdg_raster_pipeline& pipeline,
    render_id shading_model_id,
    const std::function<std::unique_ptr<shading_model_base>()>& creator)
{
    assert(m_pipeline.vertex_shader == nullptr);

    m_pipeline = pipeline;

    auto* material_manager = render_device::instance().get_material_manager();

    if (material_manager->get_shading_model(shading_model_id) == nullptr)
    {
        material_manager->set_shading_model(shading_model_id, creator());
    }

    m_shading_model = shading_model_id;
}

void material::set_pipeline_impl(
    const rdg_raster_pipeline& visibility_pipeline,
    const rdg_compute_pipeline& material_resolve_pipeline,
    render_id shading_model_id,
    const std::function<std::unique_ptr<shading_model_base>()>& creator)
{
    assert(m_pipeline.vertex_shader == nullptr);

    m_pipeline = visibility_pipeline;

    auto* material_manager = render_device::instance().get_material_manager();

    std::uint32_t pipeline_id =
        material_manager->add_material_resolve_pipeline(material_resolve_pipeline);

    if (material_manager->get_shading_model(shading_model_id) == nullptr)
    {
        material_manager->set_shading_model(shading_model_id, creator());
    }

    m_resolve_pipeline = pipeline_id;
    m_shading_model = shading_model_id;
}

void material::mark_dirty()
{
    assert(get_constant_data().second != 0);

    if (m_dirty)
    {
        return;
    }

    m_dirty = true;

    render_device::instance().get_material_manager()->mark_dirty(m_material_id);
}
} // namespace violet