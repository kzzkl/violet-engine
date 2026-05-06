#include "graphics/material.hpp"
#include "graphics/material_manager.hpp"

namespace violet
{
material::material() noexcept
{
    auto* material_manager = render_device::instance().get_material_manager();
    m_material_id = material_manager->add_material(this);

    mark_dirty(DIRTY_FLAG_ALL);

    auto& device = render_device::instance();

    m_raster_pipeline = {
        .rasterizer_state =
            device.get_rasterizer_state<RHI_CULL_MODE_BACK, RHI_POLYGON_MODE_FILL>(),
        .depth_stencil_state = device.get_depth_stencil_state<true, true, RHI_COMPARE_OP_GREATER>(),
        .primitive_topology = RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };
}

material::~material()
{
    auto* material_manager = render_device::instance().get_material_manager();
    material_manager->remove_material(m_material_id);

    if (m_resolve_pipeline_id != 0)
    {
        material_manager->remove_material_resolve_pipeline(m_resolve_pipeline_id);
    }
}

void material::set_cull_mode(rhi_cull_mode cull_mode)
{
    if (m_raster_pipeline.rasterizer_state->cull_mode != cull_mode)
    {
        auto& device = render_device::instance();
        m_raster_pipeline.rasterizer_state = device.get_rasterizer_state(
            cull_mode,
            m_raster_pipeline.rasterizer_state->polygon_mode);

        mark_dirty(DIRTY_FLAG_PIPELINE);
    }
}

void material::set_polygon_mode(rhi_polygon_mode polygon_mode)
{
    if (m_raster_pipeline.rasterizer_state->polygon_mode != polygon_mode)
    {
        auto& device = render_device::instance();
        m_raster_pipeline.rasterizer_state = device.get_rasterizer_state(
            m_raster_pipeline.rasterizer_state->cull_mode,
            polygon_mode);

        mark_dirty(DIRTY_FLAG_PIPELINE);
    }
}

void material::set_primitive_topology(rhi_primitive_topology primitive_topology)
{
    if (m_raster_pipeline.primitive_topology != primitive_topology)
    {
        m_raster_pipeline.primitive_topology = primitive_topology;
        mark_dirty(DIRTY_FLAG_PIPELINE);
    }
}

void material::set_shadow_cull_mode(shadow_cull_mode cull_mode)
{
    if (m_shadow_cull_mode != cull_mode)
    {
        m_shadow_cull_mode = cull_mode;
        mark_dirty(DIRTY_FLAG_SHADOW_CULL_MODE);
    }
}

void material::update()
{
    auto* material_manager = render_device::instance().get_material_manager();

    if (m_dirty_flags & DIRTY_FLAG_PIPELINE)
    {
        auto* material_manager = render_device::instance().get_material_manager();

        rhi_shader* resolve_shader = get_resolve_shader({});
        if (resolve_shader != nullptr && m_resolve_pipeline.compute_shader != resolve_shader)
        {
            if (m_resolve_pipeline_id != 0)
            {
                material_manager->remove_material_resolve_pipeline(m_resolve_pipeline_id);
            }

            m_resolve_pipeline.compute_shader = resolve_shader;

            m_resolve_pipeline_id =
                material_manager->add_material_resolve_pipeline(m_resolve_pipeline);

            m_dirty_flags |= DIRTY_FLAG_CONSTANT;
        }

        std::vector<std::wstring> defines;
        if (get_opacity_cutoff())
        {
            defines.emplace_back(L"-DVIOLET_OPACITY_CUTOFF");
        }

        switch (get_material_path())
        {
        case MATERIAL_PATH_FORWARD:
            defines.emplace_back(L"-DVIOLET_MATERIAL_PATH_FORWARD");
            break;
        case MATERIAL_PATH_DEFERRED:
            defines.emplace_back(L"-DVIOLET_MATERIAL_PATH_DEFERRED");
            break;
        default:
            break;
        }

        m_raster_pipeline.vertex_shader = get_vertex_shader(defines);
        m_raster_pipeline.geometry_shader = get_geometry_shader(defines);
        m_raster_pipeline.fragment_shader = get_fragment_shader(defines);
    }

    std::uint32_t shadow_batch = 0;
    shadow_batch |= get_opacity_cutoff() ? 1 : 0;
    switch (m_shadow_cull_mode)
    {
    case SHADOW_CULL_MODE_NONE:
        shadow_batch |= RHI_CULL_MODE_NONE << 1;
        break;
    case SHADOW_CULL_MODE_BACK:
        shadow_batch |= RHI_CULL_MODE_BACK << 1;
        break;
    case SHADOW_CULL_MODE_FRONT:
        shadow_batch |= RHI_CULL_MODE_FRONT << 1;
        break;
    default:
        shadow_batch |= m_raster_pipeline.rasterizer_state->cull_mode << 1;
        break;
    }

    if (m_shadow_batch != shadow_batch)
    {
        m_shadow_batch = shadow_batch;
        m_dirty_flags |= DIRTY_FLAG_CONSTANT;
    }

    if (m_dirty_flags & DIRTY_FLAG_CONSTANT)
    {
        auto [data, size] =
            get_constant_data(m_shading_model, m_resolve_pipeline_id, m_shadow_batch);
        material_manager->update_constant(m_material_id, data, size);
    }

    m_dirty_flags = 0;
}

void material::set_shading_model_impl(
    render_id shading_model_id,
    const std::function<std::unique_ptr<shading_model_base>()>& creator)
{
    auto* material_manager = render_device::instance().get_material_manager();

    if (material_manager->get_shading_model(shading_model_id) == nullptr)
    {
        material_manager->set_shading_model(shading_model_id, creator());
    }

    m_shading_model = shading_model_id;

    mark_dirty(DIRTY_FLAG_CONSTANT);
}

void material::mark_dirty(dirty_flags dirty_flags)
{
    if (m_dirty_flags == 0)
    {
        render_device::instance().get_material_manager()->mark_dirty(m_material_id);
    }

    m_dirty_flags |= dirty_flags;
}
} // namespace violet