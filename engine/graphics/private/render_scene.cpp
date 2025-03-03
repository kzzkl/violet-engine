#include "graphics/render_scene.hpp"
#include "gpu_buffer_uploader.hpp"
#include "graphics/material_manager.hpp"

namespace violet
{
render_scene::render_scene()
{
    auto& device = render_device::instance();

    m_gpu_buffer_uploader = std::make_unique<gpu_buffer_uploader>(64 * 1024);

    m_group_buffer = std::make_unique<raw_buffer>(rhi_buffer_desc{
        .size = sizeof(std::uint32_t) * 4 * 1024,
        .flags = RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST,
    });

    m_scene_parameter = device.create_parameter(shader::scene);

    m_scene_data.material_buffer =
        device.get_material_manager()->get_material_buffer()->get_srv()->get_bindless();
    m_scene_data.mesh_buffer = m_meshes.get_buffer()->get_srv()->get_bindless();
    m_scene_data.instance_buffer = m_instances.get_buffer()->get_srv()->get_bindless();
    m_scene_data.light_buffer = m_lights.get_buffer()->get_srv()->get_bindless();
    m_scene_data.group_buffer = m_group_buffer->get_srv()->get_bindless();

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

    for (render_id instance_id : mesh.instances)
    {
        remove_instance_from_group(instance_id);
        add_instance_to_group(instance_id, mesh.geometry, m_instances[instance_id].material);

        m_instances.mark_dirty(instance_id);
    }
}

render_id render_scene::add_instance(render_id mesh_id)
{
    render_id instance_id = m_instances.add();

    m_instances[instance_id] = {
        .mesh_id = mesh_id,
        .group_id = INVALID_RENDER_ID,
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

    remove_instance_from_group(instance_id);

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
        auto& mesh_info = m_meshes[instance_info.mesh_id];

        remove_instance_from_group(instance_id);
        add_instance_to_group(instance_id, mesh_info.geometry, material);

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

bool render_scene::update()
{
    update_mesh();
    update_instance();
    update_light();

    if (m_scene_states & RENDER_SCENE_STAGE_GROUP_DIRTY)
    {
        update_group_buffer();
    }

    if (m_scene_states & RENDER_SCENE_STAGE_DATA_DIRTY)
    {
        m_scene_parameter->set_uniform(0, &m_scene_data, sizeof(shader::scene_data));
    }

    m_scene_states = 0;

    return !m_gpu_buffer_uploader->empty();
}

void render_scene::record(rhi_command* command)
{
    std::vector<raw_buffer*> buffers = {
        m_meshes.get_buffer(),
        m_instances.get_buffer(),
        m_group_buffer.get(),
    };

    std::vector<rhi_buffer_barrier> barriers;
    barriers.reserve(buffers.size());

    for (raw_buffer* buffer : buffers)
    {
        rhi_buffer_barrier barrier = {
            .buffer = buffer->get_rhi(),
            .src_stages = RHI_PIPELINE_STAGE_COMPUTE | RHI_PIPELINE_STAGE_VERTEX |
                          RHI_PIPELINE_STAGE_FRAGMENT,
            .src_access = RHI_ACCESS_SHADER_READ,
            .dst_stages = RHI_PIPELINE_STAGE_TRANSFER,
            .dst_access = RHI_ACCESS_TRANSFER_WRITE,
            .offset = 0,
            .size = buffer->get_size(),
        };

        barriers.push_back(barrier);
    }

    command->set_pipeline_barrier(barriers.data(), barriers.size(), nullptr, 0);

    m_gpu_buffer_uploader->record(command);

    for (rhi_buffer_barrier& barrier : barriers)
    {
        std::swap(barrier.src_stages, barrier.dst_stages);
        std::swap(barrier.src_access, barrier.dst_access);
    }

    command->set_pipeline_barrier(barriers.data(), barriers.size(), nullptr, 0);
}

void render_scene::add_instance_to_group(
    render_id instance_id,
    const geometry* geometry,
    const material* material)
{
    assert(geometry != nullptr && material != nullptr);

    render_id batch_id = 0;
    render_id group_id = 0;

    std::uint64_t pipeline_hash =
        hash::city_hash_64(&material->get_pipeline(), sizeof(rdg_raster_pipeline));
    auto batch_iter = m_pipeline_to_batch.find(pipeline_hash);
    if (batch_iter == m_pipeline_to_batch.end())
    {
        batch_id = m_batch_allocator.allocate();
        if (batch_id >= m_batches.size())
        {
            m_batches.resize(batch_id + 1);
        }
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

    auto group_iter = std::find_if(
        batch.groups.begin(),
        batch.groups.end(),
        [geometry, this](const render_id& group_id)
        {
            return m_groups[group_id].geometry == geometry;
        });

    if (group_iter == batch.groups.end())
    {
        group_id = m_group_allocator.allocate();
        if (group_id >= m_groups.size())
        {
            m_groups.resize(group_id + 1);
        }

        auto& group = m_groups[group_id];
        group.vertex_buffers.clear();

        auto& device = render_device::instance();
        for (const auto& vertex_attribute :
             device.get_vertex_attributes(material->get_pipeline().vertex_shader))
        {
            group.vertex_buffers.push_back(geometry->get_vertex_buffer(vertex_attribute));
        }
        group.index_buffer = geometry->get_index_buffer();
        group.geometry = geometry;
        group.instance_count = 1;
        group.id = group_id;
        group.batch = batch_id;

        batch.groups.push_back(group_id);
    }
    else
    {
        group_id = (*group_iter);
        ++m_groups[group_id].instance_count;
    }

    m_instances[instance_id].group_id = group_id;

    m_scene_states |= RENDER_SCENE_STAGE_GROUP_DIRTY;
}

void render_scene::remove_instance_from_group(render_id instance_id)
{
    render_id group_id = m_instances[instance_id].group_id;

    if (group_id == INVALID_RENDER_ID)
    {
        return;
    }

    auto& group = m_groups[group_id];

    --group.instance_count;

    if (group.instance_count == 0)
    {
        auto& batch = m_batches[group.batch];

        batch.groups.erase(std::find(batch.groups.begin(), batch.groups.end(), group_id));

        if (batch.groups.empty())
        {
            m_batch_allocator.free(group.batch);
        }

        m_group_allocator.free(group_id);
    }

    m_scene_states |= RENDER_SCENE_STAGE_GROUP_DIRTY;
}

void render_scene::update_mesh()
{
    auto upload_mesh = [&](render_mesh& mesh, render_id id, std::size_t index)
    {
        shader::mesh_data mesh_data = {
            .model_matrix = mesh.model_matrix,
        };

        m_gpu_buffer_uploader->upload(
            m_meshes.get_buffer()->get_rhi(),
            &mesh_data,
            sizeof(shader::mesh_data),
            index * sizeof(shader::mesh_data));
    };

    m_meshes.update(upload_mesh);

    if (m_scene_data.mesh_buffer != m_meshes.get_buffer()->get_srv()->get_bindless())
    {
        m_scene_data.mesh_buffer = m_meshes.get_buffer()->get_srv()->get_bindless();
        m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;
    }
}

void render_scene::update_instance()
{
    auto upload_instance = [&](render_instance& instance, render_id id, std::size_t index)
    {
        shader::instance_data instance_data = {
            .mesh_index = static_cast<std::uint32_t>(m_meshes.get_index(instance.mesh_id)),
            .vertex_offset = instance.vertex_offset,
            .index_offset = instance.index_offset,
            .index_count = instance.index_count,
            .group_index = static_cast<std::uint32_t>(instance.group_id),
            .material_address = instance.material->get_constant_address(),
        };

        m_gpu_buffer_uploader->upload(
            m_instances.get_buffer()->get_rhi(),
            &instance_data,
            sizeof(shader::instance_data),
            index * sizeof(shader::instance_data));
    };

    m_instances.update(upload_instance);

    if (m_scene_data.instance_buffer != m_instances.get_buffer()->get_srv()->get_bindless())
    {
        m_scene_data.instance_buffer = m_instances.get_buffer()->get_srv()->get_bindless();
        m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;
    }
}

void render_scene::update_light()
{
    auto upload_light = [&](render_light& light, render_id id, std::size_t index)
    {
        shader::light_data light_data = {
            .position = light.position,
            .type = light.type,
            .direction = light.direction,
            .shadow = light.shadow,
            .color = light.color,
        };

        m_gpu_buffer_uploader->upload(
            m_lights.get_buffer()->get_rhi(),
            &light_data,
            sizeof(shader::light_data),
            index * sizeof(shader::light_data));
    };

    m_lights.update(upload_light);

    if (m_scene_data.light_buffer != m_lights.get_buffer()->get_srv()->get_bindless())
    {
        m_scene_data.light_buffer = m_lights.get_buffer()->get_srv()->get_bindless();
        m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;
    }
}

void render_scene::update_group_buffer()
{
    assert(m_groups.size() * sizeof(std::uint32_t) <= m_group_buffer->get_size());

    std::uint32_t instance_offset = 0;
    std::vector<std::uint32_t> group_offsets(m_groups.size());

    for (auto& group : m_groups)
    {
        group_offsets[group.id] = group.instance_offset = instance_offset;
        instance_offset += static_cast<std::uint32_t>(group.instance_count);
    }

    m_gpu_buffer_uploader->upload(
        m_group_buffer->get_rhi(),
        group_offsets.data(),
        sizeof(std::uint32_t) * group_offsets.size(),
        0);
}

render_camera::render_camera(rhi_texture* render_target, rhi_parameter* camera_parameter)
    : m_render_target(render_target),
      m_camera_parameter(camera_parameter)
{
}
} // namespace violet