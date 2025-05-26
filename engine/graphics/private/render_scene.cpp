#include "graphics/render_scene.hpp"
#include "components/camera_component.hpp"
#include "components/camera_component_meta.hpp"
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
    m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;
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

    m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;
}

void render_scene::set_mesh_matrix(render_id mesh_id, const mat4f& matrix_m)
{
    auto& mesh = m_meshes[mesh_id];
    mesh.matrix_m = matrix_m;

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

    m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;
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

render_id render_scene::add_light()
{
    render_id light_id = m_lights.add();

    m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;

    return light_id;
}

void render_scene::remove_light(render_id light_id)
{
    m_lights.remove(light_id);

    m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;
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
    std::uint32_t geometry_buffer =
        geometry_manager->get_geometry_buffer()->get_srv()->get_bindless();
    std::uint32_t vertex_buffer = geometry_manager->get_vertex_buffer()->get_srv()->get_bindless();
    std::uint32_t index_buffer = geometry_manager->get_index_buffer()->get_srv()->get_bindless();

    if (m_scene_data.material_buffer != material_buffer ||
        m_scene_data.geometry_buffer != geometry_buffer ||
        m_scene_data.vertex_buffer != vertex_buffer || m_scene_data.index_buffer != index_buffer)
    {
        m_scene_data.material_buffer = material_buffer;
        m_scene_data.geometry_buffer = geometry_buffer;
        m_scene_data.vertex_buffer = vertex_buffer;
        m_scene_data.index_buffer = index_buffer;

        m_scene_states |= RENDER_SCENE_STAGE_DATA_DIRTY;
    }

    if (m_scene_states & RENDER_SCENE_STAGE_DATA_DIRTY)
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
}

void render_scene::add_instance_to_batch(render_id instance_id, const material* material)
{
    assert(material != nullptr);

    render_id batch_id = 0;

    auto batch_iter = m_pipeline_to_batch.find(material->get_pipeline());
    if (batch_iter == m_pipeline_to_batch.end())
    {
        batch_id = m_batches.add();
        m_pipeline_to_batch[material->get_pipeline()] = batch_id;

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

    m_scene_states |= RENDER_SCENE_STAGE_BATCH_DIRTY;
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
        m_pipeline_to_batch.erase(batch.pipeline);
        m_batches.remove(batch_id);
    }

    m_scene_states |= RENDER_SCENE_STAGE_BATCH_DIRTY;
}

bool render_scene::update_mesh(gpu_buffer_uploader* uploader)
{
    return m_meshes.update(
        [](const gpu_mesh& mesh) -> shader::mesh_data
        {
            return {
                .matrix_m = mesh.matrix_m,
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
        [&](const gpu_instance& instance) -> shader::instance_data
        {
            return {
                .mesh_index = m_meshes.get_index(instance.mesh_id),
                .geometry_index = static_cast<std::uint32_t>(
                    instance.geometry->get_submesh_id(instance.submesh_index)),
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
        [&](const gpu_light& light) -> shader::light_data
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
    if ((m_scene_states & RENDER_SCENE_STAGE_BATCH_DIRTY) == 0)
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

float render_camera::get_near() const noexcept
{
    return m_camera->near;
}

float render_camera::get_far() const noexcept
{
    return m_camera->far;
}

float render_camera::get_fov() const noexcept
{
    return m_camera->fov;
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
} // namespace violet