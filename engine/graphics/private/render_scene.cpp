#include "graphics/render_scene.hpp"
#include "graphics/gpu_buffer_uploader.hpp"

namespace violet
{
render_scene::render_scene()
{
    auto& device = render_device::instance();

    m_gpu_buffer_uploader = std::make_unique<gpu_buffer_uploader>(64 * 1024);

    m_mesh_buffer = device.create_buffer({
        .size = 64 * sizeof(shader::mesh_data),
        .flags = RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST,
    });

    m_instance_buffer = device.create_buffer({
        .size = 64 * sizeof(shader::instance_data),
        .flags = RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST,
    });

    m_group_buffer = device.create_buffer({
        .size = sizeof(std::uint32_t) * 4 * 1024,
        .flags = RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST,
    });

    m_scene_parameter = device.create_parameter(shader::scene);

    const auto& buildin_resources = device.get_buildin_resources();
    m_scene_data.material_buffer = buildin_resources.material_buffer->get_handle();
    m_scene_data.brdf_lut = buildin_resources.brdf_lut->get_handle();
    m_scene_data.point_repeat_sampler = buildin_resources.point_repeat_sampler->get_handle();
    m_scene_data.point_clamp_sampler = buildin_resources.point_clamp_sampler->get_handle();
    m_scene_data.linear_repeat_sampler = buildin_resources.linear_repeat_sampler->get_handle();
    m_scene_data.linear_clamp_sampler = buildin_resources.linear_clamp_sampler->get_handle();
    m_scene_data.mesh_buffer = m_mesh_buffer->get_handle();
    m_scene_data.instance_buffer = m_instance_buffer->get_handle();
    m_scene_data.group_buffer = m_group_buffer->get_handle();

    m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;
}

render_scene::~render_scene() {}

render_id render_scene::add_mesh(const render_mesh& mesh)
{
    assert(mesh.geometry != nullptr);

    render_id mesh_id = m_mesh_allocator.allocate();

    if (mesh_id >= m_meshes.size())
    {
        m_meshes.resize(mesh_id + 1);
    }

    m_meshes[mesh_id] = {
        .data = mesh,
        .states = RENDER_MESH_STAGE_VALID,
    };

    m_mesh_index_map.add(mesh_id);

    set_mesh_state(mesh_id, RENDER_MESH_STAGE_VALID | RENDER_MESH_STAGE_DIRTY);

    m_scene_data.mesh_count = get_mesh_count();
    m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;

    return mesh_id;
}

void render_scene::remove_mesh(render_id mesh_id)
{
    auto& mesh_info = m_meshes[mesh_id];

    assert(mesh_info.instances.empty() && "Mesh has instances");

    render_id last_mesh_id = m_mesh_index_map.remove(mesh_id);
    if (last_mesh_id != mesh_id)
    {
        for (render_id instance_id : m_meshes[last_mesh_id].instances)
        {
            set_instance_state(instance_id, RENDER_INSTANCE_STAGE_DIRTY);
        }
    }

    mesh_info = {};
    m_mesh_allocator.free(mesh_id);

    m_scene_data.mesh_count = get_mesh_count();
    m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;
}

void render_scene::update_mesh_model_matrix(render_id mesh_id, const mat4f& model_matrix)
{
    auto& mesh_info = m_meshes[mesh_id];
    mesh_info.data.model_matrix = model_matrix;

    set_mesh_state(mesh_id, RENDER_MESH_STAGE_DIRTY);
}

void render_scene::update_mesh_aabb(render_id mesh_id, const box3f& aabb)
{
    auto& mesh_info = m_meshes[mesh_id];
    mesh_info.data.aabb = aabb;

    set_mesh_state(mesh_id, RENDER_MESH_STAGE_DIRTY);
}

void render_scene::update_mesh_geometry(render_id mesh_id, geometry* geometry)
{
    assert(geometry != nullptr);

    auto& mesh_info = m_meshes[mesh_id];

    if (mesh_info.data.geometry == geometry)
    {
        return;
    }

    mesh_info.data.geometry = geometry;
    set_mesh_state(mesh_id, RENDER_MESH_STAGE_DIRTY);

    for (render_id instance_id : mesh_info.instances)
    {
        remove_instance_from_group(instance_id);
        add_instance_to_group(
            instance_id,
            mesh_info.data.geometry,
            m_instances[instance_id].data.material);

        set_instance_state(instance_id, RENDER_INSTANCE_STAGE_DIRTY);
    }
}

render_id render_scene::add_instance(render_id mesh_id, const render_instance& instance)
{
    render_id instance_id = m_instance_allocator.allocate();

    if (instance_id >= m_instances.size())
    {
        m_instances.resize(instance_id + 1);
    }

    m_instances[instance_id] = {
        .data = instance,
        .mesh_id = mesh_id,
    };

    m_instance_index_map.add(instance_id);

    auto& mesh_info = m_meshes[mesh_id];
    mesh_info.instances.push_back(instance_id);

    geometry* geometry = mesh_info.data.geometry;
    material* material = instance.material;
    add_instance_to_group(instance_id, geometry, material);

    set_instance_state(instance_id, RENDER_INSTANCE_STAGE_VALID | RENDER_INSTANCE_STAGE_DIRTY);

    m_scene_data.instance_count = get_instance_count();
    m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;

    return instance_id;
}

void render_scene::remove_instance(render_id instance_id)
{
    auto& instance_info = m_instances[instance_id];
    auto& mesh_info = m_meshes[instance_info.mesh_id];

    mesh_info.instances.erase(
        std::find(mesh_info.instances.begin(), mesh_info.instances.end(), instance_id));

    remove_instance_from_group(instance_id);

    m_instance_index_map.remove(instance_id);

    instance_info = {};
    m_instance_allocator.free(instance_id);

    m_scene_data.instance_count = get_instance_count();
    m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;
}

void render_scene::update_instance(render_id instance_id, const render_instance& instance)
{
    auto& instance_info = m_instances[instance_id];

    if (instance_info.data.vertex_offset != instance.vertex_offset ||
        instance_info.data.index_offset != instance.index_offset ||
        instance_info.data.index_count != instance.index_count)
    {
        instance_info.data.vertex_offset = instance.vertex_offset;
        instance_info.data.index_offset = instance.index_offset;
        instance_info.data.index_count = instance.index_count;

        set_instance_state(instance_id, RENDER_INSTANCE_STAGE_DIRTY);
    }

    if (instance_info.data.material != instance.material)
    {
        auto& mesh_info = m_meshes[instance_info.mesh_id];

        remove_instance_from_group(instance_id);
        add_instance_to_group(instance_id, mesh_info.data.geometry, instance.material);

        set_instance_state(instance_id, RENDER_INSTANCE_STAGE_DIRTY);
    }

    instance_info.data = instance;
}

render_id render_scene::add_light(const render_light& light)
{
    render_id light_id = m_light_allocator.allocate();

    if (light_id >= m_lights.size())
    {
        m_lights.resize(light_id + 1);
    }

    m_lights[light_id] = {
        .data = light,
    };

    m_light_index_map.add(light_id);

    set_light_state(light_id, RENDER_LIGHT_STAGE_VALID | RENDER_LIGHT_STAGE_DIRTY);

    m_scene_data.light_count = m_light_index_map.get_size();
    m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;

    return light_id;
}

void render_scene::remove_light(render_id light_id)
{
    render_id last_light_id = m_light_index_map.remove(light_id);
    if (last_light_id != light_id)
    {
        set_light_state(last_light_id, RENDER_LIGHT_STAGE_DIRTY);
    }

    m_light_allocator.free(light_id);

    m_scene_data.light_count = m_light_index_map.get_size();
    m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;
}

void render_scene::update_light(render_id light_id, const render_light& light)
{
    m_lights[light_id].data = light;
    set_light_state(light_id, RENDER_LIGHT_STAGE_DIRTY);
}

void render_scene::set_skybox(rhi_texture* skybox, rhi_texture* irradiance, rhi_texture* prefilter)
{
    m_scene_data.skybox = skybox->get_handle();
    m_scene_data.irradiance = irradiance->get_handle();
    m_scene_data.prefilter = prefilter->get_handle();

    m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;
}

bool render_scene::update()
{
    if (!m_dirty_meshes.empty())
    {
        update_mesh_buffer();
    }

    if (!m_dirty_instances.empty())
    {
        update_instance_buffer();
    }

    if (m_scene_states & RENDER_SCENE_STAGE_GROUP_DIRTY)
    {
        update_group_buffer();
    }

    if (m_scene_states & RENDER_SCENE_STAGE_DATA_DIRTY)
    {
        m_scene_parameter->set_constant(0, &m_scene_data, sizeof(shader::scene_data));
    }

    m_scene_states = 0;

    return !m_gpu_buffer_uploader->empty();
}

void render_scene::record(rhi_command* command)
{
    std::vector<rhi_buffer*> buffers = {
        m_mesh_buffer.get(),
        m_instance_buffer.get(),
        m_group_buffer.get()};

    std::vector<rhi_buffer_barrier> barriers;
    barriers.reserve(buffers.size());

    for (rhi_buffer* buffer : buffers)
    {
        rhi_buffer_barrier barrier = {
            .buffer = buffer,
            .src_access = RHI_ACCESS_SHADER_READ,
            .dst_access = RHI_ACCESS_TRANSFER_WRITE,
            .offset = 0,
            .size = buffer->get_buffer_size(),
        };

        barriers.push_back(barrier);
    }

    command->set_pipeline_barrier(
        RHI_PIPELINE_STAGE_COMPUTE | RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_FRAGMENT,
        RHI_PIPELINE_STAGE_TRANSFER,
        barriers.data(),
        barriers.size(),
        nullptr,
        0);

    m_gpu_buffer_uploader->record(command);

    for (rhi_buffer_barrier& barrier : barriers)
    {
        barrier.src_access = RHI_ACCESS_TRANSFER_WRITE;
        barrier.dst_access = RHI_ACCESS_SHADER_READ;
    }

    command->set_pipeline_barrier(
        RHI_PIPELINE_STAGE_TRANSFER,
        RHI_PIPELINE_STAGE_COMPUTE | RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_FRAGMENT,
        barriers.data(),
        barriers.size(),
        nullptr,
        0);
}

void render_scene::add_instance_to_group(
    render_id instance_id,
    const geometry* geometry,
    const material* material)
{
    render_id batch_id = 0;
    render_id group_id = 0;

    std::uint64_t pipeline_hash =
        hash::city_hash_64(&material->get_pipeline(), sizeof(rdg_render_pipeline));
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

void render_scene::set_mesh_state(render_id mesh_id, render_mesh_states states)
{
    auto& mesh_info = m_meshes[mesh_id];

    if ((mesh_info.states & RENDER_MESH_STAGE_DIRTY) == 0 &&
        (states & RENDER_MESH_STAGE_DIRTY) != 0)
    {
        m_dirty_meshes.push_back(mesh_id);
    }

    mesh_info.states |= states;
}

void render_scene::set_instance_state(render_id instance_id, render_instance_states states)
{
    auto& instance_info = m_instances[instance_id];

    if ((instance_info.states & RENDER_INSTANCE_STAGE_DIRTY) == 0 &&
        (states & RENDER_INSTANCE_STAGE_DIRTY) != 0)
    {
        m_dirty_instances.push_back(instance_id);
    }

    instance_info.states |= states;
}

void render_scene::set_light_state(render_id light_id, render_light_states states)
{
    auto& light_info = m_lights[light_id];

    if ((light_info.states & RENDER_LIGHT_STAGE_DIRTY) == 0 &&
        (states & RENDER_LIGHT_STAGE_DIRTY) != 0)
    {
        m_dirty_lights.push_back(light_id);
    }

    light_info.states |= states;
}

void render_scene::update_mesh_buffer()
{
    auto upload_mesh = [&](render_id mesh_id)
    {
        auto& mesh_info = m_meshes[mesh_id];

        if (!mesh_info.is_valid())
        {
            return;
        }

        shader::mesh_data mesh_data = {};
        mesh_data.model_matrix = mesh_info.data.model_matrix;

        std::size_t mesh_index = m_mesh_index_map.get_index(mesh_id);

        m_gpu_buffer_uploader->upload(
            m_mesh_buffer.get(),
            &mesh_data,
            sizeof(shader::mesh_data),
            mesh_index * sizeof(shader::mesh_data));

        mesh_info.states &= ~RENDER_MESH_STAGE_DIRTY;
    };

    std::size_t buffer_size = m_meshes.size() * sizeof(shader::mesh_data);
    if (buffer_size > m_mesh_buffer->get_buffer_size())
    {
        reserve_mesh_buffer(m_meshes.size());

        for (render_id mesh_id = 0; mesh_id < m_meshes.size(); ++mesh_id)
        {
            upload_mesh(mesh_id);
        }
    }
    else
    {
        for (render_id mesh_id : m_dirty_meshes)
        {
            upload_mesh(mesh_id);
        }
    }

    m_dirty_meshes.clear();
}

void render_scene::update_instance_buffer()
{
    auto upload_instance = [&](render_id instance_id)
    {
        auto& instance_info = m_instances[instance_id];

        if (!instance_info.is_valid())
        {
            return;
        }

        shader::instance_data instance_data = {};
        instance_data.mesh_index =
            static_cast<std::uint32_t>(m_mesh_index_map.get_index(instance_info.mesh_id));
        instance_data.vertex_offset = instance_info.data.vertex_offset;
        instance_data.index_offset = instance_info.data.index_offset;
        instance_data.index_count = instance_info.data.index_count;
        instance_data.group_index = static_cast<std::uint32_t>(instance_info.group_id);
        instance_data.material_address = instance_info.data.material->get_constant_address();

        std::size_t instance_index = m_instance_index_map.get_index(instance_id);

        m_gpu_buffer_uploader->upload(
            m_instance_buffer.get(),
            &instance_data,
            sizeof(shader::instance_data),
            instance_index * sizeof(shader::instance_data));

        instance_info.states &= ~RENDER_INSTANCE_STAGE_DIRTY;
    };

    if (m_instances.size() * sizeof(shader::instance_data) > m_instance_buffer->get_buffer_size())
    {
        reserve_instance_buffer(m_instances.size());

        for (render_id instance_id = 0; instance_id < m_instances.size(); ++instance_id)
        {
            upload_instance(instance_id);
        }
    }
    else
    {
        for (render_id instance_id : m_dirty_instances)
        {
            upload_instance(instance_id);
        }
    }

    m_dirty_instances.clear();
}

void render_scene::update_group_buffer()
{
    assert(m_groups.size() * sizeof(std::uint32_t) <= m_group_buffer->get_buffer_size());

    std::uint32_t instance_offset = 0;
    std::vector<std::uint32_t> group_offsets(m_groups.size());

    for (auto& group : m_groups)
    {
        group_offsets[group.id] = group.instance_offset = instance_offset;
        instance_offset += static_cast<std::uint32_t>(group.instance_count);
    }

    m_gpu_buffer_uploader->upload(
        m_group_buffer.get(),
        group_offsets.data(),
        sizeof(std::uint32_t) * group_offsets.size(),
        0);
}

void render_scene::reserve_mesh_buffer(std::size_t mesh_count)
{
    std::size_t requested_buffer_size = mesh_count * sizeof(shader::mesh_data);

    if (m_mesh_buffer->get_buffer_size() >= requested_buffer_size)
    {
        return;
    }

    std::size_t buffer_size =
        m_mesh_buffer ? m_mesh_buffer->get_buffer_size() : requested_buffer_size;
    while (buffer_size < requested_buffer_size)
    {
        buffer_size *= 2;
    }

    m_mesh_buffer = render_device::instance().create_buffer({
        .size = buffer_size,
        .flags = RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST,
    });

    m_scene_data.mesh_buffer = m_mesh_buffer->get_handle();
}

void render_scene::reserve_instance_buffer(std::size_t instance_count)
{
    std::size_t requested_buffer_size = instance_count * sizeof(shader::instance_data);

    if (m_instance_buffer->get_buffer_size() >= requested_buffer_size)
    {
        return;
    }

    std::size_t buffer_size =
        m_instance_buffer ? m_instance_buffer->get_buffer_size() : requested_buffer_size;
    while (buffer_size < requested_buffer_size)
    {
        buffer_size *= 2;
    }

    m_instance_buffer = render_device::instance().create_buffer({
        .size = buffer_size,
        .flags = RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST,
    });

    m_scene_data.instance_buffer = m_instance_buffer->get_handle();
}
} // namespace violet