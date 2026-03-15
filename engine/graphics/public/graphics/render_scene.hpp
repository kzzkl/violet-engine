#pragma once

#include "components/camera_component.hpp"
#include "graphics/atmosphere.hpp"
#include "graphics/geometry.hpp"
#include "graphics/gpu_array.hpp"
#include "graphics/material.hpp"
#include <unordered_map>
#include <vector>

namespace violet
{
class gpu_buffer_uploader;
class vsm_manager;
class render_scene
{
public:
    render_scene(vsm_manager* vsm_manager);
    render_scene(const render_scene&) = delete;

    ~render_scene();

    render_scene& operator=(const render_scene&) = delete;

    render_id add_mesh();
    void remove_mesh(render_id mesh_id);
    void set_mesh_flags(render_id mesh_id, std::uint32_t flags);
    void set_mesh_matrix(render_id mesh_id, const mat4f& matrix_m, const vec3f& scale);

    render_id add_instance(render_id mesh_id);
    void remove_instance(render_id instance_id);
    void set_instance_geometry(
        render_id instance_id,
        geometry* geometry,
        std::uint32_t submesh_index);
    void set_instance_material(render_id instance_id, material* material);

    render_id add_light(std::uint32_t type);
    void remove_light(render_id light_id);
    void set_light_data(
        render_id light_id,
        const vec3f& color,
        const vec3f& position,
        const vec3f& direction);
    void set_light_shadow(render_id light_id, bool cast_shadow);

    render_id add_camera();
    void remove_camera(render_id camera_id);
    void set_camera_position(render_id camera_id, const vec3f& position);
    void set_camera_background(render_id camera_id, background_type background_type);

    void set_skybox(
        rhi_texture* environment_map,
        rhi_buffer* irradiance_sh,
        rhi_texture* prefilter_map);
    void set_atmosphere(
        const atmosphere& atmosphere,
        render_id sun_id,
        rhi_texture* transmittance_lut);

    void update(gpu_buffer_uploader* uploader);

private:
    struct gpu_batch
    {
        using gpu_type = std::uint32_t;

        surface_type surface_type;
        material_path material_path;

        rdg_raster_pipeline pipeline;

        std::uint32_t instance_offset;
        std::uint32_t instance_count;
    };

    struct gpu_mesh
    {
        using gpu_type = shader::mesh_data;

        mat4f matrix_m;
        mat4f prev_matrix_m;
        vec3f scale;
        std::uint32_t flags;
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

        // when the light type is directional light, vsm_address points to the address of
        // directional_vsm_buffer. for other types, vsm_address is the id of vsm.
        render_id vsm_address{INVALID_RENDER_ID};
    };

    enum render_scene_state : std::uint8_t
    {
        RENDER_SCENE_STATE_DATA_DIRTY = 1 << 0,
        RENDER_SCENE_STATE_BATCH_DIRTY = 1 << 1,
    };
    using render_scene_states = std::uint8_t;

    struct batch_key
    {
        rdg_raster_pipeline pipeline;
        surface_type surface_type;
        material_path material_path;

        bool operator==(const batch_key& other) const noexcept
        {
            return pipeline == other.pipeline && surface_type == other.surface_type &&
                   material_path == other.material_path;
        }
    };

    struct batch_hash
    {
        std::uint64_t operator()(const batch_key& key) const noexcept
        {
            rhi_raster_pipeline_desc desc = key.pipeline.get_desc(nullptr);

            std::uint64_t hash0 = hash::xx_hash(&desc, sizeof(rhi_raster_pipeline_desc));
            std::uint64_t hash1 = hash::xx_hash(&key.surface_type, sizeof(surface_type));
            std::uint64_t hash2 = hash::xx_hash(&key.material_path, sizeof(material_path));

            return hash::combine(hash::combine(hash0, hash1), hash2);
        }
    };

    struct gpu_directional_vsm
    {
        using gpu_type = std::array<std::uint32_t, 16>;

        struct vsm
        {
            render_id vsm_id;
            render_id camera_id;
        };

        std::array<vsm, 16> vsms;
    };

    struct camera_data
    {
        struct vsm
        {
            render_id vsm_id;
            render_id light_id;
        };

        render_id camera_id;
        vec3f position;

        std::vector<vsm> vsms;

        background_type background_type{BACKGROUND_TYPE_SKYBOX};
        rhi_ptr<rhi_texture> environment_map;
        rhi_ptr<rhi_buffer> irradiance_sh;
        rhi_ptr<rhi_texture> prefilter_map;
    };

    struct vsm_data
    {
        render_id light_id;
        render_id camera_id;

        bool dirty;
    };

    void add_instance_to_batch(render_id instance_id, const material* material);
    void remove_instance_from_batch(render_id instance_id);

    void add_vsm_by_camera(render_id camera_id);
    void remove_vsm_by_camera(render_id camera_id);

    bool update_mesh(gpu_buffer_uploader* uploader);
    bool update_instance(gpu_buffer_uploader* uploader);
    bool update_light(gpu_buffer_uploader* uploader);
    bool update_batch(gpu_buffer_uploader* uploader);
    void update_vsm();

    gpu_dense_array<gpu_mesh> m_meshes;
    std::vector<render_id> m_matrix_dirty_meshes;

    gpu_dense_array<gpu_instance> m_instances;

    struct light_mapping
    {
        render_id light_id;
        bool cast_shadow{false};
    };
    index_allocator m_light_allocator;
    std::vector<light_mapping> m_light_mappings;

    gpu_dense_array<gpu_light> m_shadow_casting_lights;
    gpu_dense_array<gpu_light> m_non_shadow_casting_lights;

    gpu_sparse_array<gpu_batch> m_batches;
    std::unordered_map<batch_key, render_id, batch_hash> m_pipeline_to_batch;

    std::vector<std::pair<rdg_compute_pipeline, std::uint32_t>> m_material_resolve_pipelines;
    std::vector<std::pair<shading_model_base*, std::uint32_t>> m_shading_models;

    std::vector<camera_data> m_cameras;
    index_allocator m_camera_allocator;

    gpu_sparse_array<gpu_directional_vsm> m_vsm_directional_buffer;
    std::unordered_map<render_id, vsm_data> m_vsms;

    std::uint32_t m_instance_capacity{1};

    render_scene_states m_scene_states{0};
    shader::scene_data m_scene_data{};
    rhi_ptr<rhi_parameter> m_scene_parameter;

    vsm_manager* m_vsm_manager;

    rhi_texture* m_environment_map;
    rhi_buffer* m_irradiance_sh;
    rhi_texture* m_prefilter_map;

    atmosphere m_atmosphere;
    render_id m_sun_id{INVALID_RENDER_ID};
    vec3f m_sun_direction;
    vec3f m_sun_irradiance;
    rhi_texture* m_transmittance_lut;

    friend class render_context;
};

class camera_component;
class camera_component_meta;
class render_context
{
public:
    render_context(const camera_component* camera, const camera_component_meta* camera_meta);

    render_id get_camera_id() const noexcept;
    camera_type get_camera_type() const noexcept;
    float get_camera_near() const noexcept;
    float get_camera_far() const noexcept;
    float get_camera_perspective_fov() const noexcept;
    float get_camera_orthographic_size() const noexcept;
    const vec3f& get_camera_position() const noexcept;
    const mat4f& get_camera_matrix_v() const noexcept;
    const mat4f& get_camera_matrix_p() const noexcept;
    const mat4f& get_camera_matrix_vp() const noexcept;
    const mat4f& get_camera_matrix_vp_no_jitter() const noexcept;
    background_type get_background_type() const noexcept;

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

    std::uint32_t get_mesh_count() const noexcept
    {
        return m_scene->m_meshes.get_size();
    }

    std::uint32_t get_instance_count() const noexcept
    {
        return m_scene->m_instances.get_size();
    }

    std::uint32_t get_light_count(bool cast_shadow) const noexcept
    {
        return cast_shadow ? m_scene->m_shadow_casting_lights.get_size() :
                             m_scene->m_non_shadow_casting_lights.get_size();
    }

    std::uint32_t get_batch_count() const noexcept
    {
        return 4 * 1024;
    }

    std::uint32_t get_instance_capacity() const noexcept
    {
        return m_scene->m_instance_capacity;
    }

    std::uint32_t get_batch_capacity() const noexcept
    {
        return 4 * 1024;
    }

    rhi_buffer* get_vsm_directional_buffer() const noexcept
    {
        return m_scene->m_vsm_directional_buffer.get_buffer()->get_rhi();
    }

    rhi_buffer* get_vsm_buffer() const noexcept;
    rhi_buffer* get_vsm_virtual_page_table() const noexcept;
    rhi_buffer* get_vsm_physical_page_table() const noexcept;
    rhi_texture* get_vsm_physical_shadow_map_static() const noexcept;
    rhi_texture* get_vsm_physical_shadow_map_final() const noexcept;
    rhi_texture* get_vsm_hzb() const noexcept;

    rhi_parameter* get_camera_parameter() const noexcept;
    rhi_parameter* get_scene_parameter() const noexcept;

    const atmosphere& get_atmosphere() const noexcept
    {
        return m_scene->m_atmosphere;
    }

    rhi_texture* get_environment_map() const noexcept
    {
        return m_environment_map;
    }

    rhi_texture* get_prefilter_map() const noexcept
    {
        return m_prefilter_map;
    }

    rhi_buffer* get_irradiance_sh() const noexcept
    {
        return m_irradiance_sh;
    }

    rhi_texture* get_transmittance_lut() const noexcept
    {
        return m_scene->m_transmittance_lut;
    }

    vec2f get_sun_transmittance_lut_uv() const noexcept
    {
        return m_sun_transmittance_lut_uv;
    }

    std::uint32_t get_sun_id(bool& cast_shadow) const noexcept;

    vec3f get_sun_direction() const noexcept
    {
        return m_scene->m_sun_direction;
    }

    vec3f get_sun_irradiance() const noexcept
    {
        return m_scene->m_sun_irradiance;
    }

    template <typename Functor>
    void each_batch(surface_type surface_type, material_path material_path, Functor&& functor) const
    {
        m_scene->m_batches.each(
            [&](render_id id, const render_scene::gpu_batch& batch)
            {
                if (batch.surface_type != surface_type || batch.material_path != material_path ||
                    batch.instance_count == 0)
                {
                    return;
                }

                functor(id, batch.pipeline, batch.instance_offset, batch.instance_count);
            });
    }

    template <typename Functor>
    void each_material_resolve_pipeline(Functor&& functor) const
    {
        for (render_id pipeline_id = 1; pipeline_id < m_scene->m_material_resolve_pipelines.size();
             ++pipeline_id)
        {
            const auto& [pipeline, instance_count] =
                m_scene->m_material_resolve_pipelines[pipeline_id];
            if (instance_count > 0)
            {
                functor(pipeline_id, pipeline);
            }
        }
    }

    template <typename Functor>
    void each_shading_model(Functor&& functor) const
    {
        for (render_id shading_model_id = 1; shading_model_id < m_scene->m_shading_models.size();
             ++shading_model_id)
        {
            const auto& [shading_model, instance_count] =
                m_scene->m_shading_models[shading_model_id];
            if (instance_count > 0)
            {
                functor(shading_model_id, shading_model);
            }
        }
    }

private:
    rhi_texture* m_render_target;
    rhi_viewport m_viewport;
    std::vector<rhi_scissor_rect> m_scissor_rects;

    rhi_texture* m_environment_map;
    rhi_buffer* m_irradiance_sh;
    rhi_texture* m_prefilter_map;

    const render_scene* m_scene;
    const camera_component* m_camera;
    const camera_component_meta* m_camera_meta;

    vec2f m_sun_transmittance_lut_uv;
};
} // namespace violet