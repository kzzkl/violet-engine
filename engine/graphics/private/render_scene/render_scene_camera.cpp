#include "graphics/render_scene/render_scene_camera.hpp"
#include <algorithm>

namespace violet
{
render_id render_scene_camera::add_camera()
{
    render_id camera_id = m_camera_allocator.allocate();

    if (m_cameras.size() <= camera_id)
    {
        m_cameras.resize(camera_id + 1);
    }

    m_cameras[camera_id] = {
        .position = {},
        .moved = true,
        .valid = true,
    };

    m_added_cameras.push_back(camera_id);

    return camera_id;
}

void render_scene_camera::remove_camera(render_id camera_id)
{
    m_removed_cameras.push_back(camera_id);

    {
        auto iter = std::ranges::find(m_added_cameras, camera_id);
        if (iter != m_added_cameras.end())
        {
            m_added_cameras.erase(iter);
        }
    }

    m_cameras[camera_id].valid = false;
}

void render_scene_camera::set_camera_position(render_id camera_id, const vec3f& position)
{
    auto& camera = m_cameras[camera_id];
    assert(camera.valid);

    camera.position = position;
    camera.moved = true;
}

void render_scene_camera::set_camera_background(
    render_id camera_id,
    background_type background_type)
{
    auto& camera = m_cameras[camera_id];
    assert(camera.valid);

    if (camera.background_type == background_type)
    {
        return;
    }

    camera.background_type = background_type;

    if (background_type == BACKGROUND_TYPE_SKYBOX)
    {
        camera.environment_map = nullptr;
        camera.irradiance_sh = nullptr;
        camera.prefilter_map = nullptr;
    }
    else if (background_type == BACKGROUND_TYPE_ATMOSPHERE)
    {
        auto& device = render_device::instance();

        camera.environment_map = device.create_texture({
            .extent = {.width = 64, .height = 64, .depth = 1},
            .format = RHI_FORMAT_R11G11B10_FLOAT,
            .flags = RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_CUBE,
            .level_count = 1,
            .layer_count = 6,
        });
        camera.environment_map->set_name("Environment Map");

        camera.irradiance_sh = device.create_buffer({
            .size = 9 * sizeof(vec4f),
            .flags = RHI_BUFFER_STORAGE,
        });
        camera.irradiance_sh->set_name("Irradiance SH");

        rhi_extent prefilter_map_extent = {.width = 64, .height = 64, .depth = 1};
        camera.prefilter_map = device.create_texture({
            .extent = prefilter_map_extent,
            .format = RHI_FORMAT_R11G11B10_FLOAT,
            .flags = RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_CUBE,
            .level_count = rhi_get_level_count(prefilter_map_extent),
            .layer_count = 6,
        });
        camera.prefilter_map->set_name("Prefilter Map");
    }
}

void render_scene_camera::update(render_scene_context& context, gpu_buffer_uploader& uploader) {}

void render_scene_camera::reset()
{
    for (auto camera_id : m_removed_cameras)
    {
        m_camera_allocator.free(camera_id);
    }

    m_added_cameras.clear();
    m_removed_cameras.clear();
}
} // namespace violet