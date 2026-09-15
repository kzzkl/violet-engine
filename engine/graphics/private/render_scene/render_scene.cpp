#include "graphics/render_scene/render_scene.hpp"
#include "components/camera_component_meta.hpp"
#include "gpu_buffer_uploader.hpp"
#include "graphics/geometry_manager.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/render_scene/render_scene_camera.hpp"
#include "graphics/render_scene/render_scene_environment.hpp"
#include "graphics/render_scene/render_scene_light.hpp"
#include "graphics/render_scene/render_scene_mesh.hpp"
#include "graphics/render_scene/render_scene_sdf.hpp"
#include "graphics/render_scene/render_scene_shadow.hpp"

namespace violet
{
render_scene::render_scene(vsm_manager* vsm_manager)
{
    auto& device = render_device::instance();

    m_scene_parameter = device.create_parameter(shader::scene);

    m_context.add_module<render_scene_camera>();
    m_context.add_module<render_scene_mesh>();
    m_context.add_module<render_scene_light>();
    m_context.add_module<render_scene_shadow>(vsm_manager);
    m_context.add_module<render_scene_environment>();
    m_context.add_module<render_scene_sdf>();
}

render_scene::~render_scene() {}

void render_scene::end_frame(gpu_buffer_uploader* uploader)
{
    shader::scene_data scene_data = m_context.scene_data;

    m_context.update(*uploader);

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
    m_context.reset();
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

    m_scene_info.parameter = m_scene->m_scene_parameter.get();
}
} // namespace violet