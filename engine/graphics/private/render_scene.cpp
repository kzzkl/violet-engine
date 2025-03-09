#include "graphics/render_scene.hpp"
#include "gpu_buffer_uploader.hpp"
#include "graphics/geometry_manager.hpp"
#include "graphics/material_manager.hpp"

namespace violet
{
render_scene::render_scene()
{
    auto& device = render_device::instance();

    m_scene_parameter = device.create_parameter(shader::scene);
    m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;
}

render_scene::~render_scene() {}

render_id render_scene::add_mesh()
{
    render_id mesh_id = m_meshes.add();

    m_scene_data.mesh_count = get_mesh_count();
    m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;

    return mesh_id;
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

    m_scene_data.mesh_count = get_mesh_count();
    m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;
}

void render_scene::set_mesh_model_matrix(render_id mesh_id, const mat4f& model_matrix)
{
    auto& mesh = m_meshes[mesh_id];
    mesh.model_matrix = model_matrix;

    m_meshes.mark_dirty(mesh_id);
}

void render_scene::set_mesh_aabb(render_id mesh_id, const box3f& aabb)
{
    auto& mesh = m_meshes[mesh_id];
    mesh.aabb = aabb;

    m_meshes.mark_dirty(mesh_id);
}

void render_scene::set_mesh_geometry(render_id mesh_id, geometry* geometry)
{
    assert(geometry != nullptr);

    auto& mesh = m_meshes[mesh_id];

    if (mesh.geometry == geometry)
    {
        return;
    }

    mesh.geometry = geometry;
    m_meshes.mark_dirty(mesh_id);
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

    m_scene_data.instance_count = get_instance_count();
    m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;

    return instance_id;
}

void render_scene::remove_instance(render_id instance_id)
{
    auto& instance = m_instances[instance_id];
    auto& mesh = m_meshes[instance.mesh_id];

    mesh.instances.erase(std::find(mesh.instances.begin(), mesh.instances.end(), instance_id));

    remove_instance_from_batch(instance_id);

    m_instances.remove(instance_id);

    m_scene_data.instance_count = get_instance_count();
    m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;
}

void render_scene::set_instance_data(
    render_id instance_id,
    std::uint32_t vertex_offset,
    std::uint32_t index_offset,
    std::uint32_t index_count)
{
    auto& instance = m_instances[instance_id];

    if (instance.vertex_offset != vertex_offset || instance.index_offset != index_offset ||
        instance.index_count != index_count)
    {
        instance.vertex_offset = vertex_offset;
        instance.index_offset = index_offset;
        instance.index_count = index_count;

        m_instances.mark_dirty(instance_id);
    }
}

void render_scene::set_instance_material(render_id instance_id, material* material)
{
    auto& instance_info = m_instances[instance_id];

    if (instance_info.material != material)
    {
        remove_instance_from_batch(instance_id);
        add_instance_to_batch(instance_id, material);

        m_instances.mark_dirty(instance_id);

        instance_info.material = material;
    }
}

render_id render_scene::add_light()
{
    render_id light_id = m_lights.add();

    m_scene_data.light_count = get_light_count();
    m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;

    return light_id;
}

void render_scene::remove_light(render_id light_id)
{
    m_lights.remove(light_id);
}

void render_scene::set_light_data(
    render_id light_id,
    std::uint32_t type,
    const vec3f& color,
    const vec3f& position,
    const vec3f& direction)
{
    auto& light = m_lights[light_id];
    light.type = type;
    light.color = color;
    light.position = position;
    light.direction = direction;

    m_lights.mark_dirty(light_id);
}

void set_light_shadow(render_id light_id, bool cast_shadow) {}

void render_scene::set_skybox(
    texture_cube* skybox,
    texture_cube* irradiance,
    texture_cube* prefilter)
{
    m_scene_data.skybox = skybox->get_srv(RHI_TEXTURE_DIMENSION_CUBE)->get_bindless();
    m_scene_data.irradiance = irradiance->get_srv(RHI_TEXTURE_DIMENSION_CUBE)->get_bindless();
    m_scene_data.prefilter = prefilter->get_srv(RHI_TEXTURE_DIMENSION_CUBE)->get_bindless();

    m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;
}

void render_scene::update(gpu_buffer_uploader* uploader)
{
    m_scene_states |= update_mesh(uploader) ? RENDER_SCENE_STAGE_DATA_DIRTY : 0;
    m_scene_states |= update_instance(uploader) ? RENDER_SCENE_STAGE_DATA_DIRTY : 0;
    m_scene_states |= update_light(uploader) ? RENDER_SCENE_STAGE_DATA_DIRTY : 0;
    m_scene_states |= update_batch(uploader) ? RENDER_SCENE_STAGE_DATA_DIRTY : 0;

    auto* material_manager = render_device::instance().get_material_manager();
    auto* geometry_manager = render_device::instance().get_geometry_manager();

    std::uint32_t material_buffer =
        material_manager->get_material_buffer()->get_srv()->get_bindless();
    std::uint32_t vertex_buffer = geometry_manager->get_vertex_buffer()->get_srv()->get_bindless();
    std::uint32_t index_buffer = geometry_manager->get_index_buffer()->get_srv()->get_bindless();

    if (m_scene_data.material_buffer != material_buffer ||
        m_scene_data.vertex_buffer != vertex_buffer || m_scene_data.index_buffer != index_buffer)
    {
        m_scene_data.material_buffer = material_buffer;
        m_scene_data.vertex_buffer = vertex_buffer;
        m_scene_data.index_buffer = index_buffer;

        m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;
    }

    if (m_scene_states & RENDER_SCENE_STAGE_DATA_DIRTY)
    {
        m_scene_data.mesh_buffer = m_meshes.get_buffer()->get_srv()->get_bindless();
        m_scene_data.instance_buffer = m_instances.get_buffer()->get_srv()->get_bindless();
        m_scene_data.light_buffer = m_lights.get_buffer()->get_srv()->get_bindless();
        m_scene_data.batch_buffer = m_batches.get_buffer()->get_srv()->get_bindless();

        m_scene_parameter->set_uniform(0, &m_scene_data, sizeof(shader::scene_data));
    }

    m_scene_states = 0;
}

void render_scene::add_instance_to_batch(render_id instance_id, const material* material)
{
    assert(material != nullptr);

    render_id batch_id = 0;
    render_id group_id = 0;

    std::uint64_t pipeline_hash =
        hash::city_hash_64(&material->get_pipeline(), sizeof(rdg_raster_pipeline));
    auto batch_iter = m_pipeline_to_batch.find(pipeline_hash);
    if (batch_iter == m_pipeline_to_batch.end())
    {
        batch_id = m_batches.add();
        m_pipeline_to_batch[pipeline_hash] = batch_id;

        auto& batch = m_batches[batch_id];
        batch.material_type = material->get_type();
        batch.pipeline = material->get_pipeline();
    }
    else
    {
        batch_id = batch_iter->second;
    }

    auto& batch = m_batches[batch_id];
    ++batch.instance_count;

    m_instances[instance_id].batch_id = batch_id;
}

void render_scene::remove_instance_from_batch(render_id instance_id)
{
    render_id batch_id = m_instances[instance_id].batch_id;

    if (batch_id == INVALID_RENDER_ID)
    {
        return;
    }

    auto& batch = m_batches[batch_id];
    --batch.instance_count;

    if (batch.instance_count == 0)
    {
        m_batches.remove(batch_id);
    }
}

bool render_scene::update_mesh(gpu_buffer_uploader* uploader)
{
    geometry_manager* geometry_manager = render_device::instance().get_geometry_manager();
    return m_meshes.update(
        [&](const render_mesh& mesh) -> shader::mesh_data
        {
            box3f world_aabb = box::transform(mesh.aabb, mesh.model_matrix);
            auto buffer_addresses = geometry_manager->get_buffer_addresses(mesh.geometry->get_id());

            return {
                .model_matrix = mesh.model_matrix,
                .aabb_min = world_aabb.min,
                .flags = 0,
                .aabb_max = world_aabb.max,
                .index_offset = buffer_addresses[GEOMETRY_BUFFER_INDEX] / 4,
                .position_address = buffer_addresses[GEOMETRY_BUFFER_POSITION],
                .normal_address = buffer_addresses[GEOMETRY_BUFFER_NORMAL],
                .tangent_address = buffer_addresses[GEOMETRY_BUFFER_TANGENT],
                .texcoord_address = buffer_addresses[GEOMETRY_BUFFER_TEXCOORD],
                .custom_addresses =
                    {
                        buffer_addresses[GEOMETRY_BUFFER_CUSTOM_0],
                        buffer_addresses[GEOMETRY_BUFFER_CUSTOM_1],
                        buffer_addresses[GEOMETRY_BUFFER_CUSTOM_2],
                        buffer_addresses[GEOMETRY_BUFFER_CUSTOM_3],
                    },
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

bool render_scene::update_instance(gpu_buffer_uploader* uploader)
{
    material_manager* material_manager = render_device::instance().get_material_manager();

    return m_instances.update(
        [&](const render_instance& instance) -> shader::instance_data
        {
            return {
                .mesh_index = static_cast<std::uint32_t>(m_meshes.get_index(instance.mesh_id)),
                .vertex_offset = instance.vertex_offset,
                .index_offset = instance.index_offset,
                .index_count = instance.index_count,
                .batch_index = static_cast<std::uint32_t>(instance.batch_id),
                .material_address =
                    material_manager->get_material_constant_address(instance.material->get_id()),
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
    return m_lights.update(
        [&](const render_light& light) -> shader::light_data
        {
            return {
                .position = light.position,
                .type = light.type,
                .direction = light.direction,
                .shadow = light.shadow,
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
    std::uint32_t instance_offset = 0;
    for (std::size_t i = 0; i < m_batches.get_size(); ++i)
    {
        auto& batch = m_batches[i];
        batch.instance_offset = instance_offset;
        instance_offset += batch.instance_count;

        m_batches.mark_dirty(i);
    }

    return m_batches.update(
        [](const render_batch& batch) -> std::uint32_t
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

render_camera::render_camera(rhi_texture* render_target, rhi_parameter* camera_parameter)
    : m_render_target(render_target),
      m_camera_parameter(camera_parameter)
{
}
} // namespace violet