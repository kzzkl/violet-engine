#include "graphics/render_scene/render_scene.hpp"
#include "components/camera_component_meta.hpp"
#include "gpu_buffer_uploader.hpp"
#include "graphics/geometry_manager.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/render_scene/render_scene_camera.hpp"
#include "graphics/render_scene/render_scene_environment.hpp"
#include "graphics/render_scene/render_scene_light.hpp"
#include "graphics/render_scene/render_scene_mesh.hpp"
#include "graphics/render_scene/render_scene_shadow.hpp"

namespace violet
{
render_scene::render_scene(vsm_manager* vsm_manager)
{
    auto& device = render_device::instance();

    m_scene_parameter = device.create_parameter(shader::scene);

    m_camera_module = std::make_unique<render_scene_camera>();
    m_mesh_module = std::make_unique<render_scene_mesh>();
    m_light_module = std::make_unique<render_scene_light>();
    m_shadow_module = std::make_unique<render_scene_shadow>(vsm_manager);
    m_environment_module = std::make_unique<render_scene_environment>();

    m_context = {
        .camera_module = m_camera_module.get(),
        .mesh_module = m_mesh_module.get(),
        .light_module = m_light_module.get(),
        .shadow_module = m_shadow_module.get(),
        .environment_module = m_environment_module.get(),
    };
}

render_scene::~render_scene() {}

void render_scene::end_frame(gpu_buffer_uploader* uploader)
{
    shader::scene_data scene_data = m_context.scene_data;

    m_camera_module->update(m_context, *uploader);
    m_mesh_module->update(m_context, *uploader);
    m_light_module->update(m_context, *uploader);
    m_shadow_module->update(m_context, *uploader);
    m_environment_module->update(m_context, *uploader);

    auto* material_manager = render_device::instance().get_material_manager();
    auto* geometry_manager = render_device::instance().get_geometry_manager();

    m_context.scene_data.material_buffer =
        material_manager->get_material_buffer()->get_srv()->get_bindless();
    m_context.scene_data.geometry_buffer =
        geometry_manager->get_geometry_buffer()->get_srv()->get_bindless();
    m_context.scene_data.vertex_buffer =
        geometry_manager->get_vertex_buffer()->get_srv()->get_bindless();
    m_context.scene_data.index_buffer =
        geometry_manager->get_index_buffer()->get_srv()->get_bindless();
    m_context.scene_data.cluster_buffer =
        geometry_manager->get_cluster_buffer()->get_srv()->get_bindless();

    if (scene_data != m_context.scene_data)
    {
        m_scene_parameter->set_uniform(0, &m_context.scene_data, sizeof(shader::scene_data));
    }
}

void render_scene::reset_states()
{
    m_camera_module->reset();
    m_mesh_module->reset();
    m_light_module->reset();
    m_shadow_module->reset();
    m_environment_module->reset();
}

render_context::render_context(
    const camera_component* camera,
    const camera_component_meta* camera_meta)
    : m_scene(camera_meta->scene)
{
    std::visit(
        [&](auto&& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, rhi_swapchain*>)
            {
                m_render_target = arg->get_texture();
            }
            else if constexpr (std::is_same_v<T, rhi_texture*>)
            {
                m_render_target = arg;
            }
        },
        camera->render_target);

    rhi_extent extent = m_render_target->get_extent();

    if (camera->viewport.width == 0.0f || camera->viewport.height == 0.0f)
    {
        m_viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(extent.width),
            .height = static_cast<float>(extent.height),
            .min_depth = 0.0f,
            .max_depth = 1.0f,
        };
    }
    else
    {
        m_viewport = camera->viewport;
    }

    if (camera->scissor_rects.empty())
    {
        m_scissor_rects.push_back({
            .min_x = 0,
            .min_y = 0,
            .max_x = extent.width,
            .max_y = extent.height,
        });
    }
    else
    {
        m_scissor_rects = camera->scissor_rects;
    }

    m_camera_info.id = camera_meta->id;
    m_camera_info.type = camera->type;
    m_camera_info.near = camera->near;
    m_camera_info.far = camera->far;
    m_camera_info.perspective_fov = camera->perspective.fov;
    m_camera_info.orthographic_size = camera->orthographic.size;
    m_camera_info.position = camera_meta->position;
    m_camera_info.matrix_v = camera_meta->matrix_v;
    m_camera_info.matrix_p = camera_meta->matrix_p;
    m_camera_info.matrix_vp = camera_meta->matrix_vp;
    m_camera_info.matrix_vp_no_jitter = camera_meta->matrix_vp_no_jitter;
    m_camera_info.background = camera->background;
    m_camera_info.parameter = camera_meta->parameter.get();

    auto& mesh_module = camera_meta->scene->get_module<render_scene_mesh>();
    m_scene_info.instance_count = mesh_module.get_instance_count();
    m_scene_info.draw_call_capacity = mesh_module.get_draw_call_capacity();
    m_scene_info.draw_call_count = mesh_module.get_draw_call_count();
    m_scene_info.batch_capacity = mesh_module.get_batch_capacity();

    auto& light_module = camera_meta->scene->get_module<render_scene_light>();
    m_scene_info.light_count = light_module.get_light_count();

    auto& shadow_module = camera_meta->scene->get_module<render_scene_shadow>();
    m_scene_info.shadow_light_buffer = shadow_module.get_shadow_light_buffer();
    m_scene_info.shadow_light_count = shadow_module.get_shadow_light_count();
    m_scene_info.vsm_buffer = shadow_module.get_vsm_buffer();
    m_scene_info.vsm_count = shadow_module.get_vsm_count();
    m_scene_info.vsm_clipmap_buffer = shadow_module.get_vsm_clipmap_buffer();
    m_scene_info.vsm_hzb = shadow_module.get_vsm_hzb();
    m_scene_info.vsm_virtual_page_table = shadow_module.get_vsm_virtual_page_table();
    m_scene_info.vsm_physical_page_table = shadow_module.get_vsm_physical_page_table();
    m_scene_info.vsm_physical_shadow_map_static =
        shadow_module.get_vsm_physical_shadow_map_static();
    m_scene_info.vsm_physical_shadow_map_final = shadow_module.get_vsm_physical_shadow_map_final();

    auto& environment_module = camera_meta->scene->get_module<render_scene_environment>();
    m_scene_info.atmosphere = environment_module.get_atmosphere();
    m_scene_info.sun_direction = environment_module.get_sun_direction();
    m_scene_info.sun_irradiance = environment_module.get_sun_irradiance();
    m_scene_info.transmittance_lut = environment_module.get_transmittance_lut();
    m_scene_info.multi_scattering_lut = environment_module.get_multi_scattering_lut();
    m_scene_info.atmosphere_diry = environment_module.is_atmosphere_dirty();

    if (camera->background == BACKGROUND_TYPE_SKYBOX)
    {
        m_scene_info.environment_map = environment_module.get_environment_map();
        m_scene_info.irradiance_sh = environment_module.get_irradiance_sh();
        m_scene_info.prefilter_map = environment_module.get_prefilter_map();
    }
    else if (camera->background == BACKGROUND_TYPE_ATMOSPHERE)
    {
        const auto& camera_module = camera_meta->scene->get_module<render_scene_camera>();
        const auto& camera_data = camera_module.get_camera(camera_meta->id);

        m_scene_info.environment_map = camera_data.environment_map.get();
        m_scene_info.irradiance_sh = camera_data.irradiance_sh.get();
        m_scene_info.prefilter_map = camera_data.prefilter_map.get();
    }

    m_scene_info.parameter = m_scene->m_scene_parameter.get();
}
} // namespace violet