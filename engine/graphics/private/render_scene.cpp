#include "graphics/render_scene.hpp"
#include "components/camera_component_meta.hpp"
#include "components/light_component.hpp"
#include "gpu_buffer_uploader.hpp"
#include "graphics/geometry_manager.hpp"
#include "graphics/graphics_config.hpp"
#include "graphics/material_manager.hpp"
#include "virtual_shadow_map/vsm_manager.hpp"
#include <algorithm>

namespace violet
{
render_camera::render_camera(
    const camera_component* camera,
    const camera_component_meta* camera_meta)
    : m_camera(camera),
      m_camera_meta(camera_meta)
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

    rhi_texture_extent extent = m_render_target->get_extent();

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
}

render_id render_camera::get_id() const noexcept
{
    return m_camera_meta->id;
}

const mat4f& render_camera::get_matrix_v() const noexcept
{
    return m_camera_meta->matrix_v;
}

const mat4f& render_camera::get_matrix_p() const noexcept
{
    return m_camera_meta->matrix_p;
}

rhi_texture* render_camera::get_hzb() const noexcept
{
    return m_camera_meta->hzb.get();
}

rhi_parameter* render_camera::get_camera_parameter() const noexcept
{
    return m_camera_meta->parameter.get();
}

render_scene::render_scene(vsm_manager* vsm_manager)
    : m_vsm_manager(vsm_manager)
{
    auto& device = render_device::instance();

    m_scene_parameter = device.create_parameter(shader::scene);
    m_scene_states |= RENDER_SCENE_STATE_DATA_DIRTY;
}

render_scene::~render_scene() {}

render_id render_scene::add_mesh()
{
    m_scene_states |= RENDER_SCENE_STATE_DATA_DIRTY;
    return m_meshes.add();
}

void render_scene::remove_mesh(render_id mesh_id)
{
    auto& mesh = m_meshes[mesh_id];

    assert(mesh.instances.empty() && "Mesh has instances");

    render_id last_mesh_id = m_meshes.remove(mesh_id);
    if (last_mesh_id != mesh_id)
    {
        for (render_id instance_id : m_meshes[last_mesh_id].instances)
        {
            m_instances.mark_dirty(instance_id);
        }
    }

    mesh = {};

    m_scene_states |= RENDER_SCENE_STATE_DATA_DIRTY;
}

void render_scene::set_mesh_matrix(render_id mesh_id, const mat4f& matrix_m, const vec3f& scale)
{
    auto& mesh = m_meshes[mesh_id];
    mesh.matrix_m = matrix_m;
    mesh.scale = scale;

    m_meshes.mark_dirty(mesh_id);

    m_matrix_dirty_meshes.push_back(mesh_id);
}

render_id render_scene::add_instance(render_id mesh_id)
{
    render_id instance_id = m_instances.add();

    m_instances[instance_id] = {
        .mesh_id = mesh_id,
        .batch_id = INVALID_RENDER_ID,
    };

    auto& mesh = m_meshes[mesh_id];
    mesh.instances.push_back(instance_id);

    m_scene_states |= RENDER_SCENE_STATE_DATA_DIRTY;

    return instance_id;
}

void render_scene::remove_instance(render_id instance_id)
{
    auto& instance = m_instances[instance_id];
    auto& mesh = m_meshes[instance.mesh_id];

    mesh.instances.erase(std::ranges::find(mesh.instances, instance_id));

    remove_instance_from_batch(instance_id);

    m_instances.remove(instance_id);

    m_scene_states |= RENDER_SCENE_STATE_DATA_DIRTY;
}

void render_scene::set_instance_geometry(
    render_id instance_id,
    geometry* geometry,
    std::uint32_t submesh_index)
{
    assert(geometry != nullptr && submesh_index < geometry->get_submeshes().size());

    auto& instance = m_instances[instance_id];

    if (instance.geometry == geometry && instance.submesh_index == submesh_index)
    {
        return;
    }

    instance.geometry = geometry;
    instance.submesh_index = submesh_index;

    m_instances.mark_dirty(instance_id);
}

void render_scene::set_instance_material(render_id instance_id, material* material)
{
    assert(material != nullptr);

    auto& instance_info = m_instances[instance_id];

    if (instance_info.material != material)
    {
        remove_instance_from_batch(instance_id);
        add_instance_to_batch(instance_id, material);

        m_instances.mark_dirty(instance_id);

        instance_info.material = material;
    }
}

render_id render_scene::add_light(std::uint32_t type)
{
    render_id light_id = m_lights.add();

    auto& light = m_lights[light_id];
    light.type = type;

    m_scene_states |= RENDER_SCENE_STATE_DATA_DIRTY;

    return light_id;
}

void render_scene::remove_light(render_id light_id)
{
    set_light_shadow(light_id, false);

    m_lights.remove(light_id);

    m_scene_states |= RENDER_SCENE_STATE_DATA_DIRTY;
}

void render_scene::set_light_data(
    render_id light_id,
    const vec3f& color,
    const vec3f& position,
    const vec3f& direction)
{
    auto& light = m_lights[light_id];
    light.color = color;
    light.position = position;
    light.direction = direction;

    if (light.cast_shadow())
    {
        if (light.type == LIGHT_DIRECTIONAL)
        {
            auto& vsms = m_directional_vsm_buffer[light.vsm_address].vsms;

            for (const auto& camera : m_cameras)
            {
                if (camera.camera_id != INVALID_RENDER_ID)
                {
                    m_vsms.at(vsms[camera.camera_id].vsm_id).dirty = true;
                }
            }
        }
    }

    m_lights.mark_dirty(light_id);
}

void render_scene::set_light_shadow(render_id light_id, bool cast_shadow)
{
    auto& light = m_lights[light_id];

    if ((light.cast_shadow() && cast_shadow) || (!light.cast_shadow() && !cast_shadow))
    {
        return;
    }

    if (cast_shadow)
    {
        add_vsm_by_light(light_id);
    }
    else
    {
        remove_vsm_by_light(light_id);
    }

    m_lights.mark_dirty(light_id);
}

render_id render_scene::add_camera()
{
    render_id camera_id = m_camera_allocator.allocate();

    if (m_cameras.size() <= camera_id)
    {
        m_cameras.resize(camera_id + 1);
    }

    m_cameras[camera_id].camera_id = camera_id;

    add_vsm_by_camera(camera_id);

    return camera_id;
}

void render_scene::remove_camera(render_id camera_id)
{
    assert(m_cameras.size() > camera_id && m_cameras[camera_id].camera_id != INVALID_RENDER_ID);

    remove_vsm_by_camera(camera_id);

    m_cameras[camera_id].camera_id = INVALID_RENDER_ID;
    m_cameras[camera_id].vsms.clear();

    m_camera_allocator.free(camera_id);
}

void render_scene::set_camera_position(render_id camera_id, const vec3f& position)
{
    auto& camera = m_cameras[camera_id];

    if (camera.position == position)
    {
        return;
    }

    camera.position = position;

    for (auto& vsm : camera.vsms)
    {
        m_vsms.at(vsm.vsm_id).dirty = true;
    }
}

void render_scene::set_skybox(
    texture_cube* skybox,
    texture_cube* irradiance,
    texture_cube* prefilter)
{
    m_scene_data.skybox = skybox->get_srv(RHI_TEXTURE_DIMENSION_CUBE)->get_bindless();
    m_scene_data.irradiance = irradiance->get_srv(RHI_TEXTURE_DIMENSION_CUBE)->get_bindless();
    m_scene_data.prefilter = prefilter->get_srv(RHI_TEXTURE_DIMENSION_CUBE)->get_bindless();

    m_scene_states |= RENDER_SCENE_STATE_DATA_DIRTY;
}

void render_scene::update(gpu_buffer_uploader* uploader)
{
    m_scene_states |= update_mesh(uploader) ? RENDER_SCENE_STATE_DATA_DIRTY : 0;
    m_scene_states |= update_instance(uploader) ? RENDER_SCENE_STATE_DATA_DIRTY : 0;
    m_scene_states |= update_light(uploader) ? RENDER_SCENE_STATE_DATA_DIRTY : 0;
    m_scene_states |= update_batch(uploader) ? RENDER_SCENE_STATE_DATA_DIRTY : 0;

    auto* material_manager = render_device::instance().get_material_manager();
    auto* geometry_manager = render_device::instance().get_geometry_manager();

    std::uint32_t material_buffer =
        material_manager->get_material_buffer()->get_srv()->get_bindless();
    std::uint32_t geometry_buffer =
        geometry_manager->get_geometry_buffer()->get_srv()->get_bindless();
    std::uint32_t vertex_buffer = geometry_manager->get_vertex_buffer()->get_srv()->get_bindless();
    std::uint32_t index_buffer = geometry_manager->get_index_buffer()->get_srv()->get_bindless();
    std::uint32_t cluster_buffer =
        geometry_manager->get_cluster_buffer()->get_srv()->get_bindless();
    std::uint32_t directional_vsm_buffer =
        m_directional_vsm_buffer.get_buffer()->get_srv()->get_bindless();

    if (m_scene_data.material_buffer != material_buffer ||
        m_scene_data.geometry_buffer != geometry_buffer ||
        m_scene_data.vertex_buffer != vertex_buffer || m_scene_data.index_buffer != index_buffer ||
        m_scene_data.cluster_buffer != cluster_buffer ||
        m_scene_data.directional_vsm_buffer != directional_vsm_buffer)
    {
        m_scene_data.material_buffer = material_buffer;
        m_scene_data.geometry_buffer = geometry_buffer;
        m_scene_data.vertex_buffer = vertex_buffer;
        m_scene_data.index_buffer = index_buffer;
        m_scene_data.cluster_buffer = cluster_buffer;
        m_scene_data.directional_vsm_buffer = directional_vsm_buffer;

        m_scene_states |= RENDER_SCENE_STATE_DATA_DIRTY;
    }

    if (m_scene_states & RENDER_SCENE_STATE_DATA_DIRTY)
    {
        m_scene_data.mesh_buffer = m_meshes.get_buffer()->get_srv()->get_bindless();
        m_scene_data.mesh_count = get_mesh_count();
        m_scene_data.instance_buffer = m_instances.get_buffer()->get_srv()->get_bindless();
        m_scene_data.instance_count = get_instance_count();
        m_scene_data.light_buffer = m_lights.get_buffer()->get_srv()->get_bindless();
        m_scene_data.light_count = get_light_count();
        m_scene_data.batch_buffer = m_batches.get_buffer()->get_srv()->get_bindless();

        m_scene_parameter->set_uniform(0, &m_scene_data, sizeof(shader::scene_data));
    }

    m_scene_states = 0;

    update_vsm();
}

rhi_buffer* render_scene::get_vsm_buffer() const noexcept
{
    return m_vsm_manager->get_vsm_buffer();
}

rhi_buffer* render_scene::get_vsm_virtual_page_table() const noexcept
{
    return m_vsm_manager->get_vsm_virtual_page_table();
}

rhi_buffer* render_scene::get_vsm_physical_page_table() const noexcept
{
    return m_vsm_manager->get_vsm_physical_page_table();
}

rhi_texture* render_scene::get_vsm_physical_texture() const noexcept
{
    return m_vsm_manager->get_vsm_physical_texture();
}

void render_scene::add_instance_to_batch(render_id instance_id, const material* material)
{
    assert(material != nullptr);

    render_id batch_id = 0;

    batch_key key = {
        .pipeline = material->get_pipeline(),
        .surface_type = material->get_surface_type(),
        .material_path = material->get_material_path(),
    };

    auto batch_iter = m_pipeline_to_batch.find(key);
    if (batch_iter == m_pipeline_to_batch.end())
    {
        batch_id = m_batches.add();
        m_pipeline_to_batch[key] = batch_id;

        auto& batch = m_batches[batch_id];
        batch.surface_type = material->get_surface_type();
        batch.material_path = key.material_path;
        batch.pipeline = key.pipeline;
    }
    else
    {
        batch_id = batch_iter->second;
    }

    auto& instance = m_instances[instance_id];
    instance.batch_id = batch_id;

    auto& batch = m_batches[batch_id];
    const auto& submesh = instance.geometry->get_submesh(instance.submesh_index);
    batch.instance_count +=
        submesh.has_cluster() ? static_cast<std::uint32_t>(submesh.clusters.size()) : 1;

    auto [pipeline_id, shading_model_id] = material->get_material_info();

    if (pipeline_id != 0)
    {
        if (pipeline_id >= m_material_resolve_pipelines.size())
        {
            m_material_resolve_pipelines.resize(pipeline_id + 1);
        }

        auto& [pipeline, instance_count] = m_material_resolve_pipelines[pipeline_id];

        if (instance_count == 0)
        {
            auto* material_manager = render_device::instance().get_material_manager();
            pipeline = material_manager->get_material_resolve_pipeline(pipeline_id);
        }

        ++instance_count;
    }

    if (shading_model_id != 0)
    {
        if (m_shading_models.size() <= shading_model_id)
        {
            m_shading_models.resize(shading_model_id + 1);
        }

        auto& [shading_model, instance_count] = m_shading_models[shading_model_id];

        if (instance_count == 0)
        {
            auto* material_manager = render_device::instance().get_material_manager();
            shading_model = material_manager->get_shading_model(shading_model_id);
        }

        ++instance_count;
    }

    m_scene_states |= RENDER_SCENE_STATE_BATCH_DIRTY;
}

void render_scene::remove_instance_from_batch(render_id instance_id)
{
    const auto& instance = m_instances[instance_id];

    if (instance.batch_id == INVALID_RENDER_ID)
    {
        return;
    }

    auto& batch = m_batches[instance.batch_id];
    const auto& submesh = instance.geometry->get_submesh(instance.submesh_index);
    batch.instance_count -=
        submesh.has_cluster() ? static_cast<std::uint32_t>(submesh.clusters.size()) : 1;

    if (batch.instance_count == 0)
    {
        m_pipeline_to_batch.erase({
            .pipeline = batch.pipeline,
            .surface_type = batch.surface_type,
            .material_path = batch.material_path,
        });
        m_batches.remove(instance.batch_id);
    }

    auto [pipeline_id, shading_model_id] = instance.material->get_material_info();

    if (pipeline_id != 0)
    {
        --m_material_resolve_pipelines[pipeline_id].second;
    }

    if (shading_model_id != 0)
    {
        --m_shading_models[shading_model_id].second;
    }

    m_scene_states |= RENDER_SCENE_STATE_BATCH_DIRTY;
}

void render_scene::add_vsm_by_light(render_id light_id)
{
    auto& light = m_lights[light_id];

    assert(!light.cast_shadow());

    if (light.type == LIGHT_DIRECTIONAL)
    {
        light.vsm_address = m_directional_vsm_buffer.add();

        auto& directional_vsm_array = m_directional_vsm_buffer[light.vsm_address].vsms;
        directional_vsm_array.fill({.vsm_id = INVALID_RENDER_ID, .camera_id = INVALID_RENDER_ID});

        for (const auto& camera : m_cameras)
        {
            if (camera.camera_id == INVALID_RENDER_ID)
            {
                continue;
            }

            render_id vsm_id = m_vsm_manager->add_vsm(LIGHT_DIRECTIONAL);

            directional_vsm_array[camera.camera_id] = {
                .vsm_id = vsm_id,
                .camera_id = camera.camera_id,
            };

            m_vsms[vsm_id] = {
                .light_id = light_id,
                .camera_id = camera.camera_id,
                .dirty = true,
            };
        }

        m_directional_vsm_buffer.mark_dirty(light.vsm_address);
    }
    else
    {
    }
}

void render_scene::remove_vsm_by_light(render_id light_id)
{
    auto& light = m_lights[light_id];

    assert(light.cast_shadow());

    if (light.type == LIGHT_DIRECTIONAL)
    {
        auto& vsms = m_directional_vsm_buffer[light.vsm_address].vsms;

        for (auto& vsm : vsms)
        {
            if (vsm.camera_id == INVALID_RENDER_ID)
            {
                continue;
            }

            auto iter = std::ranges::find_if(
                m_cameras[vsm.camera_id].vsms,
                [&](const camera_data::vsm& vsm)
                {
                    return vsm.light_id == light_id;
                });
            m_cameras[vsm.camera_id].vsms.erase(iter);

            m_vsm_manager->remove_vsm(vsm.vsm_id);

            m_vsms.erase(vsm.vsm_id);
        }

        m_directional_vsm_buffer.remove(light.vsm_address);
    }
    else
    {
    }
}

void render_scene::add_vsm_by_camera(render_id camera_id)
{
    auto& camera = m_cameras[camera_id];

    m_lights.each(
        [&](render_id light_id, const gpu_light& light)
        {
            if (light.type != LIGHT_DIRECTIONAL || !light.cast_shadow())
            {
                return;
            }

            render_id vsm_id = m_vsm_manager->add_vsm(LIGHT_DIRECTIONAL);

            auto& light_vsms = m_directional_vsm_buffer[light.vsm_address].vsms;
            light_vsms[camera_id] = {
                .vsm_id = vsm_id,
                .camera_id = camera_id,
            };
            m_directional_vsm_buffer.mark_dirty(light.vsm_address);

            camera.vsms.push_back({
                .vsm_id = vsm_id,
                .light_id = light_id,
            });

            m_vsms[vsm_id] = {
                .light_id = light_id,
                .camera_id = camera_id,
                .dirty = true,
            };
        });
}

void render_scene::remove_vsm_by_camera(render_id camera_id)
{
    auto& camera = m_cameras[camera_id];

    for (auto& vsm : camera.vsms)
    {
        auto& light_vsms = m_directional_vsm_buffer[vsm.light_id].vsms;
        light_vsms[camera_id] = {
            .vsm_id = INVALID_RENDER_ID,
            .camera_id = INVALID_RENDER_ID,
        };
        m_directional_vsm_buffer.mark_dirty(vsm.light_id);

        m_vsm_manager->remove_vsm(vsm.vsm_id);

        m_vsms.erase(vsm.vsm_id);
    }

    camera.vsms.clear();
}

bool render_scene::update_mesh(gpu_buffer_uploader* uploader)
{
    bool need_upload = m_meshes.update(
        [](const gpu_mesh& mesh) -> shader::mesh_data
        {
            return {
                .matrix_m = mesh.matrix_m,
                .scale = {mesh.scale.x, mesh.scale.y, mesh.scale.z, vector::max(mesh.scale)},
                .prev_matrix_m = mesh.prev_matrix_m,
            };
        },
        [&](rhi_buffer* buffer, const void* data, std::size_t size, std::size_t offset)
        {
            uploader->upload(
                buffer,
                data,
                size,
                offset,
                RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_COMPUTE,
                RHI_ACCESS_SHADER_READ);
        });

    for (render_id mesh_id : m_matrix_dirty_meshes)
    {
        auto& mesh = m_meshes[mesh_id];
        mesh.prev_matrix_m = mesh.matrix_m;
        m_meshes.mark_dirty(mesh_id);
    }
    m_matrix_dirty_meshes.clear();

    return need_upload;
}

bool render_scene::update_instance(gpu_buffer_uploader* uploader)
{
    material_manager* material_manager = render_device::instance().get_material_manager();

    return m_instances.update(
        [&](const gpu_instance& instance) -> shader::instance_data
        {
            return {
                .mesh_index = m_meshes.get_index(instance.mesh_id),
                .geometry_index = static_cast<std::uint32_t>(
                    instance.geometry->get_submesh_id(instance.submesh_index)),
                .batch_index = static_cast<std::uint32_t>(instance.batch_id),
                .material_address = material_manager->get_material_constant_address(
                    instance.material->get_material_id()),
            };
        },
        [&](rhi_buffer* buffer, const void* data, std::size_t size, std::size_t offset)
        {
            uploader->upload(
                buffer,
                data,
                size,
                offset,
                RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_COMPUTE,
                RHI_ACCESS_SHADER_READ);
        });
}

bool render_scene::update_light(gpu_buffer_uploader* uploader)
{
    m_directional_vsm_buffer.update(
        [](const gpu_directional_vsm& directional_vsm)
        {
            gpu_directional_vsm::gpu_type data;
            for (std::size_t i = 0; i < data.size(); ++i)
            {
                data[i] = directional_vsm.vsms[i].vsm_id;
            }
            return data;
        },
        [&](rhi_buffer* buffer, const void* data, std::size_t size, std::size_t offset)
        {
            uploader->upload(
                buffer,
                data,
                size,
                offset,
                RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_FRAGMENT |
                    RHI_PIPELINE_STAGE_COMPUTE,
                RHI_ACCESS_SHADER_READ);
        });

    return m_lights.update(
        [&](const gpu_light& light) -> shader::light_data
        {
            return {
                .position = light.position,
                .type = light.type,
                .direction = light.direction,
                .vsm_address = light.vsm_address,
                .color = light.color,
            };
        },
        [&](rhi_buffer* buffer, const void* data, std::size_t size, std::size_t offset)
        {
            uploader->upload(
                buffer,
                data,
                size,
                offset,
                RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_FRAGMENT |
                    RHI_PIPELINE_STAGE_COMPUTE,
                RHI_ACCESS_SHADER_READ);
        });
}

bool render_scene::update_batch(gpu_buffer_uploader* uploader)
{
    if ((m_scene_states & RENDER_SCENE_STATE_BATCH_DIRTY) == 0)
    {
        return false;
    }

    std::uint32_t instance_offset = 0;
    m_batches.each(
        [&](render_id batch_id, gpu_batch& batch)
        {
            batch.instance_offset = instance_offset;
            instance_offset += batch.instance_count;
            m_batches.mark_dirty(batch_id);
        });

    // Align to 1024 to prevent frequent allocation of draw command buffers during cull pass.
    if (m_instance_capacity < instance_offset)
    {
        m_instance_capacity = (instance_offset + 1023) & ~1023;
    }
    m_instance_capacity =
        std::min(m_instance_capacity, graphics_config::get_max_draw_command_count());

    return m_batches.update(
        [](const gpu_batch& batch) -> std::uint32_t
        {
            return batch.instance_offset;
        },
        [&](rhi_buffer* buffer, const void* data, std::size_t size, std::size_t offset)
        {
            uploader->upload(
                buffer,
                data,
                size,
                offset,
                RHI_PIPELINE_STAGE_COMPUTE,
                RHI_ACCESS_SHADER_READ);
        },
        true);
}

void render_scene::update_vsm()
{
    for (auto& [vsm_id, vsm_data] : m_vsms)
    {
        if (!vsm_data.dirty)
        {
            continue;
        }

        const auto& light = m_lights[vsm_data.light_id];
        if (light.type == LIGHT_DIRECTIONAL)
        {
            const auto& camera = m_cameras[vsm_data.camera_id];
            m_vsm_manager->set_vsm(
                vsm_id,
                {
                    .light_position = light.position,
                    .light_direction = light.direction,
                    .camera_position = camera.position,
                });
        }

        vsm_data.dirty = false;
    }
}
} // namespace violet