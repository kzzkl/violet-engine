#include "graphics/material.hpp"
#include "common/utility.hpp"
#include "graphics/material_manager.hpp"
#include <format>

namespace violet
{
struct material_deferred_vs : public mesh_vs
{
    static constexpr std::string_view path = "assets/shaders/materials/material_deferred.hlsl";
};

struct material_deferred_fs : public mesh_fs
{
    static constexpr std::string_view path = "assets/shaders/materials/material_deferred.hlsl";
};

struct material_visibility_vs : public mesh_vs
{
    static constexpr std::string_view path = "assets/shaders/visibility/material_visibility.hlsl";
};

struct material_visibility_fs : public mesh_fs
{
    static constexpr std::string_view path = "assets/shaders/visibility/material_visibility.hlsl";
};

material::material(std::string_view name, std::size_t constant_size, std::string_view shader_path)
    : m_name(name)
{
    if (shader_path.empty())
    {
        m_shader_path = std::format("materials/{}.hlsli", name);
    }
    else
    {
        m_shader_path = shader_path;
    }

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

    m_constant.resize(sizeof(material_header) + constant_size);
}

material::~material()
{
    auto* material_manager = render_device::instance().get_material_manager();
    material_manager->remove_material(m_material_id);

    if (get_raster_pipeline_id() != 0)
    {
        material_manager->remove_raster_pipeline(get_raster_pipeline_id());
    }

    if (get_resolve_pipeline_id() != 0)
    {
        material_manager->remove_resolve_pipeline(get_resolve_pipeline_id());
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

void material::set_blend_state(const rhi_blend_state* blend_state)
{
    if (m_raster_pipeline.blend_state != blend_state)
    {
        m_raster_pipeline.blend_state = blend_state;
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
    auto& device = render_device::instance();

    auto* material_manager = render_device::instance().get_material_manager();

    if (m_dirty_flags & DIRTY_FLAG_PIPELINE)
    {
        std::vector<std::wstring> defines;
        if (get_opacity_cutoff())
        {
            defines.emplace_back(L"-DVIOLET_OPACITY_CUTOFF");
        }

        defines.emplace_back(std::format(L"-DMATERIAL_NAME={}", string_to_wstring(m_name)));
        defines.emplace_back(
            std::format(L"-DMATERIAL_SHADER=\"{}\"", string_to_wstring(m_shader_path)));

        rhi_shader* vertex_shader = nullptr;
        rhi_shader* fragment_shader = nullptr;

        material_path material_path = get_material_path();

        switch (material_path)
        {
        case MATERIAL_PATH_FORWARD: {
            defines.emplace_back(L"-DVIOLET_MATERIAL_PATH=MATERIAL_PATH_FORWARD");
            break;
        }
        case MATERIAL_PATH_DEFERRED: {
            defines.emplace_back(L"-DVIOLET_MATERIAL_PATH=MATERIAL_PATH_DEFERRED");
            vertex_shader = device.get_shader<material_deferred_vs>(defines);
            fragment_shader = device.get_shader<material_deferred_fs>(defines);
            break;
        }
        case MATERIAL_PATH_VISIBILITY: {
            defines.emplace_back(L"-DVIOLET_MATERIAL_PATH=MATERIAL_PATH_VISIBILITY");
            vertex_shader = device.get_shader<material_visibility_vs>(defines);
            fragment_shader = device.get_shader<material_visibility_fs>(defines);
            break;
        }
        default:
            break;
        }

        if ((vertex_shader != nullptr && m_raster_pipeline.vertex_shader != vertex_shader) ||
            (fragment_shader != nullptr && m_raster_pipeline.fragment_shader != fragment_shader))
        {
            if (m_raster_pipeline_id != 0)
            {
                material_manager->remove_raster_pipeline(m_raster_pipeline_id);
            }

            m_raster_pipeline.vertex_shader = vertex_shader;
            m_raster_pipeline.fragment_shader = fragment_shader;

            m_raster_pipeline_id = material_manager->add_raster_pipeline(m_raster_pipeline);
        }

        if (material_path == MATERIAL_PATH_VISIBILITY)
        {
            rhi_shader* resolve_shader = device.get_shader<material_resolve_cs>(defines);

            if (m_resolve_pipeline.compute_shader != resolve_shader)
            {
                auto& header = get_header();

                if (header.get_resolve_pipeline() != 0)
                {
                    material_manager->remove_resolve_pipeline(header.get_resolve_pipeline());
                }

                m_resolve_pipeline.compute_shader = resolve_shader;

                header.set_resolve_pipeline(
                    material_manager->add_resolve_pipeline(m_resolve_pipeline));

                m_dirty_flags |= DIRTY_FLAG_CONSTANT;
            }
        }
    }

    rhi_cull_mode shadow_cull_mode = RHI_CULL_MODE_NONE;
    switch (m_shadow_cull_mode)
    {
    case SHADOW_CULL_MODE_NONE:
        shadow_cull_mode = RHI_CULL_MODE_NONE;
        break;
    case SHADOW_CULL_MODE_BACK:
        shadow_cull_mode = RHI_CULL_MODE_BACK;
        break;
    case SHADOW_CULL_MODE_FRONT:
        shadow_cull_mode = RHI_CULL_MODE_FRONT;
        break;
    default:
        shadow_cull_mode = m_raster_pipeline.rasterizer_state->cull_mode;
        break;
    }

    auto& header = get_header();
    if (shadow_cull_mode != header.get_shadow_cull_mode())
    {
        header.set_shadow_cull_mode(shadow_cull_mode);
        m_dirty_flags |= DIRTY_FLAG_CONSTANT;
    }

    if (m_dirty_flags & DIRTY_FLAG_CONSTANT)
    {
        material_manager->update_constant(m_material_id, m_constant.data(), m_constant.size());
    }

    m_dirty_flags = 0;
}

void material::set_shading_model_impl(
    render_id shading_model_id,
    const std::function<std::unique_ptr<shading_model>()>& creator)
{
    auto* material_manager = render_device::instance().get_material_manager();

    if (material_manager->get_shading_model(shading_model_id) == nullptr)
    {
        material_manager->set_shading_model(shading_model_id, creator());
    }

    get_header().set_shading_model(shading_model_id);

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