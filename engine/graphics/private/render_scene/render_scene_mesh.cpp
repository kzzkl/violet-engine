#include "graphics/render_scene/render_scene_mesh.hpp"
#include "components/mesh_component.hpp"
#include "gpu_buffer_uploader.hpp"
#include "graphics/graphics_config.hpp"
#include "graphics/material_manager.hpp"
#include <algorithm>

namespace violet
{
render_id render_scene_mesh::add_mesh()
{
    return m_meshes.add();
}

void render_scene_mesh::remove_mesh(render_id mesh_id)
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
}

void render_scene_mesh::set_mesh_flags(render_id mesh_id, std::uint32_t flags)
{
    auto& mesh = m_meshes[mesh_id];

    if (mesh.flags == flags)
    {
        return;
    }

    mesh.flags = flags;

    m_meshes.mark_dirty(mesh_id);
}

void render_scene_mesh::set_mesh_matrix(
    render_id mesh_id,
    const mat4f& matrix_m,
    const vec3f& scale)
{
    auto& mesh = m_meshes[mesh_id];
    mesh.matrix_m = matrix_m;
    mesh.scale = {
        .x = scale.x,
        .y = scale.y,
        .z = scale.z,
        .w = std::max({std::abs(scale.x), std::abs(scale.y), std::abs(scale.z)}),
    };
    m_meshes.mark_dirty(mesh_id);

    m_matrix_dirty_meshes.push_back(mesh_id);
}

render_id render_scene_mesh::add_instance(render_id mesh_id)
{
    render_id instance_id = m_instances.add();

    m_instances[instance_id] = {
        .mesh_id = mesh_id,
        .batch_id = INVALID_RENDER_ID,
    };

    auto& mesh = m_meshes[mesh_id];
    mesh.instances.push_back(instance_id);

    return instance_id;
}

void render_scene_mesh::remove_instance(render_id instance_id)
{
    auto& instance = m_instances[instance_id];
    auto& mesh = m_meshes[instance.mesh_id];

    if (mesh.flags & MESH_STATIC && instance.geometry_id != INVALID_RENDER_ID)
    {
        m_invalidation_bounds.push_back(
            sphere::transform(
                m_geometries[instance.geometry_id]
                    .submeshes[instance.submesh_index]
                    .bounding_sphere,
                mesh.matrix_m,
                mesh.scale.w));
    }

    mesh.instances.erase(std::ranges::find(mesh.instances, instance_id));

    remove_instance_from_batch(instance_id);

    instance = {};
    m_instances.remove(instance_id);
}

void render_scene_mesh::set_instance_geometry(
    render_id instance_id,
    geometry* geometry,
    std::uint32_t submesh_index)
{
    assert(geometry != nullptr && submesh_index < geometry->get_submeshes().size());

    if (m_geometries.size() <= geometry->get_geometry_id())
    {
        m_geometries.resize(geometry->get_geometry_id() + 1);
    }
    m_geometries[geometry->get_geometry_id()] = geometry;

    auto& instance = m_instances[instance_id];

    if (instance.geometry_id == geometry->get_geometry_id() &&
        instance.submesh_index == submesh_index)
    {
        return;
    }

    const auto& mesh = m_meshes[instance.mesh_id];
    if (mesh.flags & MESH_STATIC)
    {
        if (instance.geometry_id != INVALID_RENDER_ID)
        {
            m_invalidation_bounds.push_back(
                sphere::transform(
                    m_geometries[instance.geometry_id]
                        .submeshes[instance.submesh_index]
                        .bounding_sphere,
                    mesh.matrix_m,
                    mesh.scale.w));
        }

        m_invalidation_bounds.push_back(
            sphere::transform(
                m_geometries[geometry->get_geometry_id()].submeshes[submesh_index].bounding_sphere,
                mesh.matrix_m,
                mesh.scale.w));
    }

    if (instance.geometry_id != INVALID_RENDER_ID &&
        m_geometries[instance.geometry_id].submeshes[instance.submesh_index].draw_call_count !=
            geometry->get_submeshes()[submesh_index].get_draw_call_count())
    {
        remove_instance_from_batch(instance_id);
        instance.geometry_id = geometry->get_geometry_id();
        instance.submesh_index = submesh_index;
        add_instance_to_batch(instance_id);
    }
    else
    {
        instance.geometry_id = geometry->get_geometry_id();
        instance.submesh_index = submesh_index;
    }

    m_instances.mark_dirty(instance_id);
}

void render_scene_mesh::set_instance_material(render_id instance_id, material* material)
{
    assert(material != nullptr);

    if (m_materials.size() <= material->get_material_id())
    {
        m_materials.resize(material->get_material_id() + 1);
    }
    m_materials[material->get_material_id()] = material;

    auto& instance = m_instances[instance_id];
    if (instance.material_id == material->get_material_id())
    {
        return;
    }

    instance.material_id = material->get_material_id();

    remove_instance_from_batch(instance_id);
    add_instance_to_batch(instance_id);

    m_instances.mark_dirty(instance_id);
}

void render_scene_mesh::update(render_scene_context& context, gpu_buffer_uploader& uploader)
{
    update_material();
    update_meshes(context, uploader);
    update_instances(context, uploader);
    update_batch(context, uploader);

    m_dirty_flags = 0;
}

void render_scene_mesh::add_instance_to_batch(render_id instance_id)
{
    auto& instance = m_instances[instance_id];
    const auto& material = m_materials[instance.material_id];

    render_id batch_id = 0;

    auto batch_iter = m_pipeline_to_batch.find(material.raster_pipeline);
    if (batch_iter == m_pipeline_to_batch.end())
    {
        batch_id = m_batches.add();
        m_pipeline_to_batch[material.raster_pipeline] = batch_id;

        auto& batch = m_batches[batch_id];
        batch.raster_pipeline_id = material.raster_pipeline;
        batch.surface_type = material.surface_type;
        batch.material_path = material.material_path;
    }
    else
    {
        batch_id = batch_iter->second;
    }

    instance.batch_id = batch_id;

    auto& batch = m_batches[batch_id];
    batch.flags |= material.opacity_cutoff ? BATCH_FLAG_OPACITY_CUTOFF : BATCH_FLAG_NONE;

    batch.draw_call_count +=
        m_geometries[instance.geometry_id].submeshes[instance.submesh_index].draw_call_count;

    render_id shading_model_id = material.shading_model;
    if (m_shading_model_reference_counts.size() <= shading_model_id)
    {
        m_shading_model_reference_counts.resize(shading_model_id + 1);
    }
    ++m_shading_model_reference_counts[shading_model_id];

    render_id resolve_pipeline_id = material.resolve_pipeline;
    if (m_resolve_pipeline_reference_counts.size() <= resolve_pipeline_id)
    {
        m_resolve_pipeline_reference_counts.resize(resolve_pipeline_id + 1);
    }
    ++m_resolve_pipeline_reference_counts[resolve_pipeline_id];

    ++m_draw_call_info.shadow_batch_instance_counts[material.shadow_batch];

    auto& material_instances = m_material_to_instances[instance.material_id];
    material_instances.push_back(instance_id);

    m_dirty_flags |= DIRTY_FLAG_BATCH;
}

void render_scene_mesh::remove_instance_from_batch(render_id instance_id)
{
    const auto& instance = m_instances[instance_id];

    if (instance.batch_id == INVALID_RENDER_ID)
    {
        return;
    }

    auto& batch = m_batches[instance.batch_id];

    batch.draw_call_count -=
        m_geometries[instance.geometry_id].submeshes[instance.submesh_index].draw_call_count;

    if (batch.draw_call_count == 0)
    {
        m_pipeline_to_batch.erase(batch.raster_pipeline_id);
        m_batches.remove(instance.batch_id);
    }

    const auto& material = m_materials[instance.material_id];

    --m_shading_model_reference_counts[material.shading_model];
    --m_resolve_pipeline_reference_counts[material.resolve_pipeline];
    --m_draw_call_info.shadow_batch_instance_counts[material.shadow_batch];

    auto iter = m_material_to_instances.find(instance.material_id);
    iter->second.erase(std::ranges::find(iter->second, instance_id));
    if (iter->second.empty())
    {
        m_material_to_instances.erase(iter);
    }

    m_dirty_flags |= DIRTY_FLAG_BATCH;
}

void render_scene_mesh::update_material()
{
    auto* material_manager = render_device::instance().get_material_manager();

    material_manager->each_dirty_material(
        [&](material* material, material::dirty_flags dirty_flags)
        {
            auto iter = m_material_to_instances.find(material->get_material_id());
            if (iter == m_material_to_instances.end())
            {
                return;
            }

            if (dirty_flags & material::DIRTY_FLAG_PIPELINE)
            {
                auto instances = iter->second;

                for (render_id instance_id : instances)
                {
                    remove_instance_from_batch(instance_id);
                }

                m_materials[material->get_material_id()] = material;

                for (render_id instance_id : instances)
                {
                    add_instance_to_batch(instance_id);
                }
            }
        });
}

void render_scene_mesh::update_meshes(render_scene_context& context, gpu_buffer_uploader& uploader)
{
    m_meshes.update(
        [](const mesh_data& mesh) -> shader::mesh_data
        {
            return {
                .matrix_m = mesh.matrix_m,
                .scale = mesh.scale,
                .prev_matrix_m = mesh.prev_matrix_m,
                .flags = mesh.flags,
            };
        },
        [&](rhi_buffer* buffer, const void* data, std::size_t size, std::size_t offset)
        {
            uploader.upload(
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

    context.scene_data.mesh_buffer = m_meshes.get_buffer()->get_srv()->get_bindless();
    context.scene_data.mesh_count = m_meshes.get_size();
}

void render_scene_mesh::update_instances(
    render_scene_context& context,
    gpu_buffer_uploader& uploader)
{
    material_manager* material_manager = render_device::instance().get_material_manager();

    m_instances.update(
        [&](const instance_data& instance) -> shader::instance_data
        {
            const auto& geometry = m_geometries[instance.geometry_id];

            return {
                .mesh_index = m_meshes.get_index(instance.mesh_id),
                .geometry_index = static_cast<std::uint32_t>(
                    geometry.submeshes[instance.submesh_index].submesh_id),
                .batch_index = static_cast<std::uint32_t>(instance.batch_id),
                .material_address =
                    material_manager->get_material_constant_address(instance.material_id),
            };
        },
        [&](rhi_buffer* buffer, const void* data, std::size_t size, std::size_t offset)
        {
            uploader.upload(
                buffer,
                data,
                size,
                offset,
                RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_COMPUTE,
                RHI_ACCESS_SHADER_READ);
        });

    context.scene_data.instance_buffer = m_instances.get_buffer()->get_srv()->get_bindless();
    context.scene_data.instance_count = m_instances.get_size();
}

void render_scene_mesh::update_batch(render_scene_context& context, gpu_buffer_uploader& uploader)
{
    if ((m_dirty_flags & DIRTY_FLAG_BATCH) == 0)
    {
        return;
    }

    m_draw_call_info.draw_call_count = 0;
    m_material_path_mask = 0;

    m_batches.each(
        [&](render_id batch_id, batch_data& batch)
        {
            batch.draw_call_offset = m_draw_call_info.draw_call_count;
            m_draw_call_info.draw_call_count += batch.draw_call_count;

            m_material_path_mask |= (1 << batch.material_path);

            m_batches.mark_dirty(batch_id);
        });

    // Align to 1024 to prevent frequent allocation of draw command buffers during cull pass.
    if (m_draw_call_info.draw_call_capacity < m_draw_call_info.draw_call_count)
    {
        m_draw_call_info.draw_call_capacity = (m_draw_call_info.draw_call_count + 1023) & ~1023;
    }
    m_draw_call_info.draw_call_capacity = std::min(
        m_draw_call_info.draw_call_capacity,
        graphics_config::get_max_draw_command_count());

    m_batches.update(
        [](const batch_data& batch) -> std::uint32_t
        {
            return batch.draw_call_offset;
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
        },
        true);

    context.scene_data.batch_buffer = m_batches.get_buffer()->get_srv()->get_bindless();
}
} // namespace violet