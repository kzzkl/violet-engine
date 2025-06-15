#pragma once

#include "graphics/geometry.hpp"
#include "graphics/gpu_array.hpp"
#include "graphics/material.hpp"
#include "graphics/resources/texture.hpp"
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
    void set_mesh_matrix(render_id mesh_id, const mat4f& matrix_m);

    render_id add_instance(render_id mesh_id);
    void remove_instance(render_id instance_id);
    void set_instance_geometry(
        render_id instance_id,
        geometry* geometry,
        std::uint32_t submesh_index);
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

    std::uint32_t get_mesh_count() const noexcept
    {
        return m_meshes.get_size();
    }

    std::uint32_t get_instance_count() const noexcept
    {
        return m_instances.get_size();
    }

    std::uint32_t get_light_count() const noexcept
    {
        return m_lights.get_size();
    }

    std::uint32_t get_batch_count() const noexcept
    {
        return 4 * 1024;
    }

    std::uint32_t get_instance_capacity() const noexcept
    {
        return m_instance_capacity;
    }

    std::uint32_t get_batch_capacity() const noexcept
    {
        return 4 * 1024;
    }

    rhi_parameter* get_scene_parameter() const noexcept
    {
        return m_scene_parameter.get();
    }

    template <typename Functor>
    void each_batch(material_type type, Functor&& functor) const
    {
        m_batches.each(
            [&](render_id id, const gpu_batch& batch)
            {
                if (batch.material_type != type || batch.instance_count == 0)
                {
                    return;
                }

                functor(id, batch.pipeline, batch.instance_offset, batch.instance_count);
            });
    }

private:
    struct gpu_batch
    {
        using gpu_type = std::uint32_t;

        material_type material_type;
        rdg_raster_pipeline pipeline;

        std::uint32_t instance_offset;
        std::uint32_t instance_count;
    };

    struct gpu_mesh
    {
        using gpu_type = shader::mesh_data;

        mat4f matrix_m;
        std::vector<render_id> instances;
    };

    struct gpu_instance
    {
        using gpu_type = shader::instance_data;

        material* material;

        geometry* geometry;
        std::uint32_t submesh_index;

        render_id mesh_id;
        render_id batch_id;
    };

    struct gpu_light
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
        RENDER_SCENE_STAGE_BATCH_DIRTY = 1 << 1,
    };
    using render_scene_states = std::uint32_t;

    struct raster_pipeline_hash
    {
        std::uint64_t operator()(const rdg_raster_pipeline& pipeline) const noexcept
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

    gpu_dense_array<gpu_mesh> m_meshes;
    gpu_dense_array<gpu_instance> m_instances;
    gpu_dense_array<gpu_light> m_lights;

    gpu_sparse_array<gpu_batch> m_batches;
    std::unordered_map<rdg_raster_pipeline, render_id, raster_pipeline_hash> m_pipeline_to_batch;

    std::uint32_t m_instance_capacity{1};

    render_scene_states m_scene_states{0};

    shader::scene_data m_scene_data{};
    rhi_ptr<rhi_parameter> m_scene_parameter;
};

class camera_component;
class camera_component_meta;
class render_camera
{
public:
    render_camera(const camera_component* camera, const camera_component_meta* camera_meta);

    float get_near() const noexcept;
    float get_far() const noexcept;
    float get_fov() const noexcept;

    const mat4f& get_matrix_v() const noexcept;
    const mat4f& get_matrix_p() const noexcept;

    const rhi_viewport& get_viewport() const noexcept
    {
        return m_viewport;
    }

    const std::vector<rhi_scissor_rect>& get_scissor_rects() const noexcept
    {
        return m_scissor_rects;
    }

    rhi_texture* get_render_target() const noexcept
    {
        return m_render_target;
    }

    rhi_texture* get_hzb() const noexcept;

    rhi_parameter* get_camera_parameter() const noexcept;

private:
    rhi_texture* m_render_target;

    rhi_viewport m_viewport;
    std::vector<rhi_scissor_rect> m_scissor_rects;

    const camera_component* m_camera;
    const camera_component_meta* m_camera_meta;
};
} // namespace violet