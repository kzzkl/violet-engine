#pragma once

#include "graphics/geometry.hpp"
#include "graphics/gpu_array.hpp"
#include "graphics/material.hpp"
#include "graphics/resources/texture.hpp"
#include "math/box.hpp"
#include <unordered_map>
#include <vector>

namespace violet
{
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

    void update(gpu_buffer_uploader* uploader);

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

    template <typename Functor>
    void each_batch(material_type type, Functor&& functor) const
    {
        for (std::size_t i = 0; i < m_batches.get_size(); ++i)
        {
            const auto& batch = m_batches[i];

            if (batch.material_type != type || batch.instance_count == 0)
            {
                continue;
            }

            functor(i, batch.pipeline, batch.instance_offset, batch.instance_count);
        }
    }

private:
    struct render_batch
    {
        using gpu_type = std::uint32_t;

        material_type material_type;
        rdg_raster_pipeline pipeline;

        std::size_t instance_offset;
        std::size_t instance_count;
    };

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
        render_id batch_id;
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
    };
    using render_scene_states = std::uint32_t;

    struct raster_pipeline_hash
    {
        std::size_t operator()(const rdg_raster_pipeline& pipeline) const noexcept
        {
            return hash::city_hash_64(pipeline);
        }
    };

    void add_instance_to_batch(render_id instance_id, const material* material);
    void remove_instance_from_batch(render_id instance_id);

    bool update_mesh(gpu_buffer_uploader* uploader);
    bool update_instance(gpu_buffer_uploader* uploader);
    bool update_light(gpu_buffer_uploader* uploader);
    bool update_batch(gpu_buffer_uploader* uploader);

    gpu_dense_array<render_mesh> m_meshes;
    gpu_dense_array<render_instance> m_instances;
    gpu_dense_array<render_light> m_lights;

    gpu_sparse_array<render_batch> m_batches;
    std::unordered_map<rdg_raster_pipeline, render_id, raster_pipeline_hash> m_pipeline_to_batch;

    render_scene_states m_scene_states{0};

    shader::scene_data m_scene_data{};
    rhi_ptr<rhi_parameter> m_scene_parameter;
};

class render_camera
{
public:
    render_camera(rhi_texture* render_target, rhi_parameter* camera_parameter);

    void set_viewport(const rhi_viewport& viewport) noexcept
    {
        m_viewport = viewport;
    }

    const rhi_viewport& get_viewport() const noexcept
    {
        return m_viewport;
    }

    void set_scissor_rects(const std::vector<rhi_scissor_rect>& scissor_rects)
    {
        m_scissor_rects = scissor_rects;
    }

    const std::vector<rhi_scissor_rect>& get_scissor_rects() const noexcept
    {
        return m_scissor_rects;
    }

    rhi_texture* get_render_target() const noexcept
    {
        return m_render_target;
    }

    rhi_parameter* get_camera_parameter() const noexcept
    {
        return m_camera_parameter;
    }

private:
    rhi_texture* m_render_target;
    rhi_parameter* m_camera_parameter;

    rhi_viewport m_viewport;
    std::vector<rhi_scissor_rect> m_scissor_rects;
};
} // namespace violet