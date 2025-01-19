#pragma once

#include "graphics/geometry.hpp"
#include "graphics/material.hpp"
#include "graphics/render_object_container.hpp"
#include "graphics/texture.hpp"
#include "math/box.hpp"

namespace violet
{
struct render_group
{
    std::vector<vertex_buffer*> vertex_buffers;
    index_buffer* index_buffer;

    const geometry* geometry;

    std::size_t instance_offset;
    std::size_t instance_count;

    render_id id;
    render_id batch;
};

struct render_batch
{
    material_type material_type;
    rdg_render_pipeline pipeline;

    std::vector<render_id> groups;
};

class gpu_buffer_uploader;
class render_scene
{
public:
    render_scene();
    render_scene(const render_scene&) = delete;

    ~render_scene();

    render_scene& operator=(const render_scene&) = delete;

    render_id add_mesh();
    void remove_mesh(render_id mesh_id);
    void set_mesh_model_matrix(render_id mesh_id, const mat4f& model_matrix);
    void set_mesh_aabb(render_id mesh_id, const box3f& aabb);
    void set_mesh_geometry(render_id mesh_id, geometry* geometry);

    render_id add_instance(render_id mesh_id);
    void remove_instance(render_id instance_id);
    void set_instance_data(
        render_id instance_id,
        std::uint32_t vertex_offset,
        std::uint32_t index_offset,
        std::uint32_t index_count);
    void set_instance_material(render_id instance_id, material* material);

    render_id add_light();
    void remove_light(render_id light_id);
    void set_light_data(
        render_id light_id,
        std::uint32_t type,
        const vec3f& color,
        const vec3f& position,
        const vec3f& direction);
    void set_light_shadow(render_id light_id, bool cast_shadow);

    void set_skybox(texture_cube* skybox, texture_cube* irradiance, texture_cube* prefilter);
    bool has_skybox() const noexcept
    {
        return m_scene_data.skybox != 0;
    }

    bool update();
    void record(rhi_command* command);

    const std::vector<render_batch>& get_batches() const noexcept
    {
        return m_batches;
    }

    const render_group& get_group(render_id group_id) const
    {
        return m_groups[group_id];
    }

    std::size_t get_mesh_count() const noexcept
    {
        return m_meshes.get_size();
    }

    std::size_t get_instance_count() const noexcept
    {
        return m_instances.get_size();
    }

    std::size_t get_light_count() const noexcept
    {
        return m_lights.get_size();
    }

    std::size_t get_group_count() const noexcept
    {
        return 4ull * 1024;
    }

    std::size_t get_mesh_capacity() const noexcept
    {
        return m_meshes.get_capacity();
    }

    std::size_t get_instance_capacity() const noexcept
    {
        return m_instances.get_capacity();
    }

    std::size_t get_group_capacity() const noexcept
    {
        return 4ull * 1024;
    }

    rhi_parameter* get_scene_parameter() const noexcept
    {
        return m_scene_parameter.get();
    }

private:
    struct render_mesh
    {
        using gpu_type = shader::mesh_data;

        mat4f model_matrix;
        box3f aabb;

        geometry* geometry;

        std::vector<render_id> instances;
    };

    struct render_instance
    {
        using gpu_type = shader::instance_data;

        std::uint32_t vertex_offset;
        std::uint32_t index_offset;
        std::uint32_t index_count;

        material* material;

        render_id mesh_id;
        render_id group_id;
    };

    struct render_light
    {
        using gpu_type = shader::light_data;

        std::uint32_t type;
        vec3f position;
        vec3f direction;
        vec3f color;
        std::uint32_t shadow;
    };

    enum render_scene_state
    {
        RENDER_SCENE_STAGE_DATA_DIRTY = 1 << 0,
        RENDER_SCENE_STAGE_GROUP_DIRTY = 1 << 1,
    };
    using render_scene_states = std::uint32_t;

    void add_instance_to_group(
        render_id instance_id,
        const geometry* geometry,
        const material* material);
    void remove_instance_from_group(render_id instance_id);

    void update_mesh();
    void update_instance();
    void update_light();
    void update_group_buffer();

    render_object_container<render_mesh> m_meshes;
    render_object_container<render_instance> m_instances;
    render_object_container<render_light> m_lights;

    std::vector<render_batch> m_batches;
    index_allocator<render_id> m_batch_allocator;
    std::unordered_map<std::uint64_t, render_id> m_pipeline_to_batch;

    std::vector<render_group> m_groups;
    index_allocator<render_id> m_group_allocator;
    std::unique_ptr<raw_buffer> m_group_buffer;

    render_scene_states m_scene_states{0};

    shader::scene_data m_scene_data{};
    rhi_ptr<rhi_parameter> m_scene_parameter;

    std::unique_ptr<gpu_buffer_uploader> m_gpu_buffer_uploader;
};
} // namespace violet