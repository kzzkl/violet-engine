#include "graphics/render_scene/render_scene_shadow.hpp"
#include "gpu_buffer_uploader.hpp"
#include "graphics/render_scene/render_scene_camera.hpp"
#include "graphics/render_scene/render_scene_light.hpp"
#include "graphics/render_scene/render_scene_mesh.hpp"
#include "virtual_shadow_map/vsm_manager.hpp"

namespace violet
{
render_scene_shadow::render_scene_shadow(vsm_manager* vsm_manager)
    : m_vsm_manager(vsm_manager)
{
}

void render_scene_shadow::add_shadow(render_id light_id)
{
    m_added_lights.push_back(light_id);
}

void render_scene_shadow::remove_shadow(render_id light_id)
{
    m_removed_lights.push_back(light_id);
}

void render_scene_shadow::update(render_scene_context& context, gpu_buffer_uploader& uploader)
{
    deallocate_vsm(context);
    allocate_vsm(context);
    update_vsm(context);
    update_shadow_addresss(uploader);
    update_clipmaps(uploader);
    update_invalidation(context, uploader);

    m_added_lights.clear();
    m_removed_lights.clear();
}

rhi_buffer* render_scene_shadow::get_vsm_buffer() const noexcept
{
    return m_vsm_manager->get_vsm_buffer();
}

std::uint32_t render_scene_shadow::get_vsm_count() const noexcept
{
    return m_vsm_manager->get_vsm_count();
}

rhi_texture* render_scene_shadow::get_vsm_hzb() const noexcept
{
    return m_vsm_manager->get_vsm_hzb();
}

rhi_buffer* render_scene_shadow::get_vsm_virtual_page_table() const noexcept
{
    return m_vsm_manager->get_vsm_virtual_page_table();
}

rhi_buffer* render_scene_shadow::get_vsm_physical_page_table() const noexcept
{
    return m_vsm_manager->get_vsm_physical_page_table();
}

rhi_texture* render_scene_shadow::get_vsm_physical_shadow_map_static() const noexcept
{
    return m_vsm_manager->get_vsm_physical_shadow_map_static();
}

rhi_texture* render_scene_shadow::get_vsm_physical_shadow_map_final() const noexcept
{
    return m_vsm_manager->get_vsm_physical_shadow_map_final();
}

void render_scene_shadow::deallocate_vsm(render_scene_context& context)
{
    for (auto light_id : m_removed_lights)
    {
        render_id shadow_address_id = INVALID_RENDER_ID;
        render_id vsm_address = INVALID_RENDER_ID;

        m_shadow_addresses.each(
            [&](render_id id, const shadow_address& shadow_address)
            {
                if (shadow_address.light_id == light_id)
                {
                    shadow_address_id = id;
                    vsm_address = shadow_address.vsm_address;
                }
            });

        assert(vsm_address != INVALID_RENDER_ID);
        assert(shadow_address_id != INVALID_RENDER_ID);

        const auto& light = context.light_module->get_light(light_id);

        if (light.type == LIGHT_DIRECTIONAL)
        {
            const auto& clipmap = m_clipmaps[vsm_address];
            for (const auto& vsm : clipmap.vsms)
            {
                if (vsm.camera_id != INVALID_RENDER_ID)
                {
                    m_vsm_manager->remove_vsm(vsm.vsm_id);
                }
            }
            m_clipmaps.remove(vsm_address);
        }
        else
        {
            m_vsm_manager->remove_vsm(vsm_address);
        }

        m_shadow_addresses.remove(shadow_address_id);

        m_lights.erase(light_id);
    }

    context.camera_module->each_removed_camera(
        [&](render_id camera_id)
        {
            m_clipmaps.each(
                [&](render_id id, clipmap& clipmap)
                {
                    if (clipmap.vsms[camera_id].vsm_id != INVALID_RENDER_ID)
                    {
                        m_vsm_manager->remove_vsm(clipmap.vsms[camera_id].vsm_id);
                        clipmap.vsms[camera_id].vsm_id = INVALID_RENDER_ID;
                    }
                });

            m_cameras.erase(camera_id);
        });
}

void render_scene_shadow::allocate_vsm(render_scene_context& context)
{
    for (auto light_id : m_added_lights)
    {
        const auto& light = context.light_module->get_light(light_id);

        render_id shadow_address_id;

        if (light.type == LIGHT_DIRECTIONAL)
        {
            render_id clipmap_id = m_clipmaps.add();
            auto& clipmap = m_clipmaps[clipmap_id];

            for (auto& vsm : clipmap.vsms)
            {
                vsm.vsm_id = INVALID_RENDER_ID;
                vsm.light_id = light_id;
                vsm.camera_id = INVALID_RENDER_ID;
            }

            for (auto& [camera_id, camera_snapshot] : m_cameras)
            {
                clipmap.vsms[camera_id] = {
                    .vsm_id = m_vsm_manager->add_vsm(LIGHT_DIRECTIONAL),
                    .light_id = light_id,
                    .camera_id = camera_id,
                };
            }

            shadow_address_id = m_shadow_addresses.add();

            auto& shadow_address = m_shadow_addresses[shadow_address_id];
            shadow_address.light_id = light_id;
            shadow_address.vsm_address = clipmap_id;

            m_shadow_addresses.mark_dirty(shadow_address_id);
            m_clipmaps.mark_dirty(clipmap_id);
        }
        else
        {
            shadow_address_id = m_shadow_addresses.add();

            auto& shadow_address = m_shadow_addresses[shadow_address_id];
            shadow_address.light_id = light_id;
            shadow_address.vsm_address = m_vsm_manager->add_vsm(light.type);

            m_shadow_addresses.mark_dirty(shadow_address_id);
        }

        m_lights[light_id] = {
            .light_id = light_id,
            .shadow_address = shadow_address_id,
        };
    }

    context.camera_module->each_added_camera(
        [&](render_id camera_id)
        {
            for (const auto& [light_id, light_snapshot] : m_lights)
            {
                const auto& light = context.light_module->get_light(light_id);
                if (light.type != LIGHT_DIRECTIONAL)
                {
                    continue;
                }

                auto& clipmap = m_clipmaps[light_snapshot.shadow_address];
                clipmap.vsms[camera_id].vsm_id = m_vsm_manager->add_vsm(LIGHT_DIRECTIONAL);
                clipmap.vsms[camera_id].camera_id = camera_id;
                m_clipmaps.mark_dirty(light_snapshot.shadow_address);
            }

            m_cameras[camera_id] = {
                .camera_id = camera_id,
            };
        });
}

void render_scene_shadow::update_vsm(render_scene_context& context)
{
    for (auto& [light_id, snapshot] : m_lights)
    {
        const auto& light = context.light_module->get_light(light_id);
        const auto& shadow_address = m_shadow_addresses[snapshot.shadow_address];

        if (light.type == LIGHT_DIRECTIONAL)
        {
            if (light.direction != snapshot.direction)
            {
                vsm_directional_light_data light_data;
                light_data.light_direction = light.direction;

                const auto& clipmap = m_clipmaps[shadow_address.vsm_address];

                for (const auto& vsm : clipmap.vsms)
                {
                    if (vsm.camera_id == INVALID_RENDER_ID)
                    {
                        continue;
                    }

                    const auto& camera = context.camera_module->get_camera(vsm.camera_id);
                    light_data.camera_position = camera.position;
                    m_vsm_manager->set_vsm(vsm.vsm_id, light_data);
                }

                snapshot.direction = light.direction;
            }
        }
        else
        {
            if (light.position != snapshot.position || light.direction != snapshot.direction)
            {
                snapshot.direction = light.direction;
                snapshot.position = light.position;
            }
        }
    }

    for (auto& [camera_id, snapshot] : m_cameras)
    {
        const auto& camera = context.camera_module->get_camera(camera_id);

        if (camera.position != snapshot.position)
        {
            m_clipmaps.each(
                [&](render_id clipmap_id, const clipmap& clipmap)
                {
                    vsm_directional_light_data light_data;
                    light_data.camera_position = camera.position;

                    auto& light_snapshot = m_lights[clipmap.vsms[camera_id].light_id];
                    light_data.light_direction = light_snapshot.direction;

                    m_vsm_manager->set_vsm(clipmap.vsms[camera_id].vsm_id, light_data);
                });

            snapshot.position = camera.position;
        }
    }
}

void render_scene_shadow::update_shadow_addresss(gpu_buffer_uploader& uploader)
{
    m_shadow_addresses.update(
        [](const shadow_address& data)
        {
            return vec2u{data.light_id, data.vsm_address};
        },
        [&](rhi_buffer* buffer, const void* data, std::size_t size, std::size_t offset)
        {
            uploader.upload(
                buffer,
                data,
                size,
                offset,
                RHI_PIPELINE_STAGE_COMPUTE,
                RHI_ACCESS_SHADER_READ);
        });
}

void render_scene_shadow::update_clipmaps(gpu_buffer_uploader& uploader)
{
    m_clipmaps.update(
        [](const clipmap& data)
        {
            clipmap::gpu_type gpu_data;

            for (std::uint32_t i = 0; i < data.vsms.size(); ++i)
            {
                gpu_data[i] = data.vsms[i].vsm_id;
            }

            return gpu_data;
        },
        [&](rhi_buffer* buffer, const void* data, std::size_t size, std::size_t offset)
        {
            uploader.upload(
                buffer,
                data,
                size,
                offset,
                RHI_PIPELINE_STAGE_COMPUTE,
                RHI_ACCESS_SHADER_READ);
        });
}

void render_scene_shadow::update_invalidation(
    render_scene_context& context,
    gpu_buffer_uploader& uploader)
{
    m_invalidations.clear();

    for (const auto& bounds : context.mesh_module->get_invalidation_bounds())
    {
        auto bounds_id = m_invalidations.add();
        m_invalidations[bounds_id].sphere = bounds;
    }

    m_invalidations.update(
        [](const page_invalidation& data)
        {
            return vec4f{
                data.sphere.center.x,
                data.sphere.center.y,
                data.sphere.center.z,
                data.sphere.radius,
            };
        },
        [&](rhi_buffer* buffer, const void* data, std::size_t size, std::size_t offset)
        {
            uploader.upload(
                buffer,
                data,
                size,
                offset,
                RHI_PIPELINE_STAGE_COMPUTE,
                RHI_ACCESS_SHADER_READ);
        });
}
} // namespace violet