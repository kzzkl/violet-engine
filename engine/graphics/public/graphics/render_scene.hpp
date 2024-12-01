#pragma once

#include "common/allocator.hpp"
#include "graphics/geometry.hpp"
#include "graphics/material.hpp"

namespace violet
{
struct render_camera
{
    rhi_parameter* camera_parameter;
    std::vector<rhi_texture*> render_targets;

    rhi_viewport viewport;
    rhi_scissor_rect scissor_rect;
};

struct render_mesh
{
    mat4f model_matrix;
    box3f aabb;

    geometry* geometry;
};

struct render_instance
{
    std::uint32_t vertex_offset;
    std::uint32_t index_offset;
    std::uint32_t index_count;

    material* material;
};

struct render_group
{
    std::vector<rhi_buffer*> vertex_buffers;
    rhi_buffer* index_buffer;

    geometry* geometry;

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

    render_id add_mesh(const render_mesh& mesh);
    void remove_mesh(render_id mesh_id);
    void update_mesh_model_matrix(render_id mesh_id, const mat4f& model_matrix);
    void update_mesh_aabb(render_id mesh_id, const box3f& aabb);

    render_id add_instance(render_id mesh_id, const render_instance& instance);
    void remove_instance(render_id instance_id);
    void update_instance(render_id instance_id, const render_instance& instance);

    void set_skybox(rhi_texture* skybox, rhi_texture* irradiance, rhi_texture* prefilter);

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
        return m_mesh_index_map.size();
    }

    std::size_t get_instance_count() const noexcept
    {
        return m_instance_index_map.size();
    }

    constexpr std::size_t get_group_count() const noexcept
    {
        return 4 * 1024;
    }

    std::size_t get_mesh_capacity() const noexcept
    {
        return m_mesh_buffer->get_buffer_size() / sizeof(shader::mesh_data);
    }

    std::size_t get_instance_capacity() const noexcept
    {
        return m_instance_buffer->get_buffer_size() / sizeof(shader::instance_data);
    }

    constexpr std::size_t get_group_capacity() const noexcept
    {
        return 4 * 1024;
    }

    rhi_parameter* get_bindless_parameter() const noexcept
    {
        return render_device::instance().get_bindless_parameter();
    }

    rhi_parameter* get_scene_parameter() const noexcept
    {
        return m_scene_parameter.get();
    }

private:
    enum render_mesh_state
    {
        RENDER_MESH_STAGE_VALID = 1 << 0,
        RENDER_MESH_STAGE_DIRTY = 1 << 1,
    };
    using render_mesh_states = std::uint32_t;

    struct render_mesh_info
    {
        render_mesh data;
        std::vector<render_id> instances;

        render_mesh_states states;

        bool is_valid() const noexcept
        {
            return states & RENDER_MESH_STAGE_VALID;
        }
    };

    enum render_instance_state
    {
        RENDER_INSTANCE_STAGE_VALID = 1 << 0,
        RENDER_INSTANCE_STAGE_DIRTY = 1 << 1,
    };
    using render_instance_states = std::uint32_t;

    struct render_instance_info
    {
        render_instance data;

        render_id mesh_id;
        render_id group_id;

        render_instance_states states;

        bool is_valid() const noexcept
        {
            return states & RENDER_INSTANCE_STAGE_VALID;
        }
    };

    enum render_scene_state
    {
        RENDER_SCENE_STAGE_DATA_DIRTY = 1 << 0,
        RENDER_SCENE_STAGE_GROUP_DIRTY = 1 << 1,
    };
    using render_scene_states = std::uint32_t;

    class bimap
    {
    public:
        void add(render_id id)
        {
            if (m_id_to_index.size() <= id)
            {
                m_id_to_index.resize(id + 1);
            }

            m_id_to_index[id] = m_index_to_id.size();
            m_index_to_id.push_back(id);
        }

        render_id remove(render_id id)
        {
            render_id last_id = m_index_to_id.back();
            if (last_id != id)
            {
                std::size_t index = m_id_to_index[id];

                m_index_to_id[index] = last_id;
                m_id_to_index[last_id] = index;
            }

            m_index_to_id.pop_back();

            return last_id;
        }

        render_id get_id(std::size_t index) const noexcept
        {
            return m_index_to_id[index];
        }

        std::size_t get_index(render_id id) const noexcept
        {
            return m_id_to_index[id];
        }

        std::size_t size() const noexcept
        {
            return m_index_to_id.size();
        }

    private:
        std::vector<std::size_t> m_id_to_index;
        std::vector<render_id> m_index_to_id;
    };

    void add_instance_to_group(render_id instance_id, geometry* geometry, material* material);
    void remove_instance_from_group(render_id instance_id);

    void set_mesh_state(render_id mesh_id, render_mesh_states states);
    void set_instance_state(render_id instance_id, render_instance_states states);

    void update_mesh_buffer();
    void update_instance_buffer();
    void update_group_buffer();

    void reserve_mesh_buffer(std::size_t mesh_count);
    void reserve_instance_buffer(std::size_t instance_count);

    std::vector<render_mesh_info> m_meshes;
    index_allocator<render_id> m_mesh_allocator;
    bimap m_mesh_index_map;
    std::vector<render_id> m_dirty_meshes;

    std::vector<render_instance_info> m_instances;
    index_allocator<render_id> m_instance_allocator;
    bimap m_instance_index_map;
    std::vector<render_id> m_dirty_instances;

    std::vector<render_batch> m_batches;
    index_allocator<render_id> m_batch_allocator;
    std::unordered_map<std::uint64_t, render_id> m_pipeline_to_batch;

    std::vector<render_group> m_groups;
    index_allocator<render_id> m_group_allocator;

    rhi_ptr<rhi_buffer> m_mesh_buffer;
    rhi_ptr<rhi_buffer> m_instance_buffer;
    rhi_ptr<rhi_buffer> m_group_buffer;

    render_scene_states m_scene_states;

    shader::scene_data m_scene_data;
    rhi_ptr<rhi_parameter> m_scene_parameter;

    std::unique_ptr<gpu_buffer_uploader> m_gpu_buffer_uploader;
};
} // namespace violet