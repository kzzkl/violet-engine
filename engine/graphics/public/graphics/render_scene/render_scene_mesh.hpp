#pragma once

#include "graphics/geometry.hpp"
#include "graphics/gpu_array.hpp"
#include "graphics/material.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/render_scene/render_scene_module.hpp"

namespace violet
{
class gpu_buffer_uploader;

class render_scene_mesh : public render_scene_module
{
public:
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

    void update(render_scene_context& context, gpu_buffer_uploader& uploader) override;
    void reset() override
    {
        m_invalidation_bounds.clear();
    }

    std::uint32_t get_mesh_count() const noexcept
    {
        return m_meshes.get_size();
    }

    std::uint32_t get_instance_count() const noexcept
    {
        return m_instances.get_size();
    }

    std::uint32_t get_draw_call_count() const noexcept
    {
        return m_draw_call_info.draw_call_count;
    }

    std::uint32_t get_draw_call_capacity() const noexcept
    {
        return m_draw_call_info.draw_call_capacity;
    }

    std::uint32_t get_batch_count() const noexcept
    {
        return m_batches.get_size();
    }

    std::uint32_t get_batch_capacity() const noexcept
    {
        return 4 * 1024;
    }

    bool has_material_path(material_path material_path) const
    {
        return m_material_path_mask & (1 << static_cast<std::uint32_t>(material_path));
    }

    const std::vector<sphere3f>& get_invalidation_bounds() const noexcept
    {
        return m_invalidation_bounds;
    }

    template <typename Functor>
    void each_batch(surface_type surface_type, material_path material_path, Functor&& functor) const
    {
        m_batches.each(
            [&](render_id id, const batch_data& batch)
            {
                if (batch.surface_type != surface_type || batch.material_path != material_path ||
                    batch.draw_call_count == 0)
                {
                    return;
                }

                auto& device = render_device::instance();

                functor(
                    id,
                    device.get_material_manager()->get_raster_pipeline(batch.raster_pipeline_id),
                    batch.draw_call_offset,
                    batch.draw_call_count);
            });
    }

    template <typename Functor>
    void each_shadow_batch(Functor&& functor) const
    {
        for (std::uint32_t index = 0; index < m_draw_call_info.shadow_batch_instance_counts.size();
             ++index)
        {
            if (m_draw_call_info.shadow_batch_instance_counts[index] == 0)
            {
                continue;
            }

            bool opacity_cutoff = index & 1;
            auto cull_mode = static_cast<rhi_cull_mode>(index >> 1);

            functor(opacity_cutoff, cull_mode);
        }
    }

    template <typename Functor>
    void each_shading_model(Functor&& functor) const
    {
        auto* material_manager = render_device::instance().get_material_manager();
        for (std::uint32_t i = 1; i < m_shading_model_reference_counts.size(); ++i)
        {
            if (m_shading_model_reference_counts[i] > 0)
            {
                functor(i, material_manager->get_shading_model(i));
            }
        }
    }

    template <typename Functor>
    void each_material_resolve_pipeline(Functor&& functor) const
    {
        auto* material_manager = render_device::instance().get_material_manager();
        for (std::uint32_t i = 1; i < m_resolve_pipeline_reference_counts.size(); ++i)
        {
            if (m_resolve_pipeline_reference_counts[i] > 0)
            {
                functor(i, material_manager->get_resolve_pipeline(i));
            }
        }
    }

private:
    struct mesh_data
    {
        using gpu_type = shader::mesh_data;

        mat4f matrix_m;
        mat4f prev_matrix_m;
        vec4f scale;
        std::uint32_t flags;
        std::vector<render_id> instances;
    };

    struct instance_data
    {
        using gpu_type = shader::instance_data;

        render_id material_id{INVALID_RENDER_ID};

        render_id geometry_id{INVALID_RENDER_ID};
        std::uint32_t submesh_index;

        render_id mesh_id{INVALID_RENDER_ID};
        render_id batch_id{INVALID_RENDER_ID};
    };

    enum batch_flag
    {
        BATCH_FLAG_NONE = 0,
        BATCH_FLAG_OPACITY_CUTOFF = 1 << 0,
    };
    using batch_flags = std::uint32_t;

    struct batch_data
    {
        using gpu_type = std::uint32_t;

        render_id raster_pipeline_id;

        surface_type surface_type;
        material_path material_path;

        batch_flags flags{BATCH_FLAG_NONE};

        std::uint32_t draw_call_offset;
        std::uint32_t draw_call_count;
    };

    struct material_snapshot
    {
        surface_type surface_type;
        material_path material_path;

        render_id raster_pipeline;
        render_id resolve_pipeline;
        render_id shading_model;

        std::uint32_t shadow_batch;
        bool opacity_cutoff;

        material_snapshot() = default;

        material_snapshot(material* material)
        {
            surface_type = material->get_surface_type();
            material_path = material->get_material_path();

            raster_pipeline = material->get_raster_pipeline_id();
            resolve_pipeline = material->get_resolve_pipeline_id();
            shading_model = material->get_shading_model_id();

            shadow_batch = material->get_shadow_batch();
            opacity_cutoff = material->get_opacity_cutoff();
        }

        bool operator==(const material_snapshot& other) const
        {
            return surface_type == other.surface_type && material_path == other.material_path &&
                   raster_pipeline == other.raster_pipeline &&
                   resolve_pipeline == other.resolve_pipeline &&
                   shading_model == other.shading_model && shadow_batch == other.shadow_batch &&
                   opacity_cutoff == other.opacity_cutoff;
        }
    };

    struct geometry_snapshot
    {
        struct submesh
        {
            std::uint32_t submesh_id;
            std::uint32_t draw_call_count;
            sphere3f bounding_sphere;
        };

        geometry_snapshot() = default;

        geometry_snapshot(geometry* geometry)
        {
            for (std::uint32_t i = 0; i < geometry->get_submeshes().size(); ++i)
            {
                submeshes.push_back({
                    .submesh_id = geometry->get_submesh_id(i),
                    .draw_call_count = geometry->get_submesh(i).get_draw_call_count(),
                    .bounding_sphere = geometry->get_bounding_sphere(i),
                });
            }
        }

        std::vector<submesh> submeshes;
    };

    void add_instance_to_batch(render_id instance_id);
    void remove_instance_from_batch(render_id instance_id);

    void update_material();
    void update_meshes(render_scene_context& context, gpu_buffer_uploader& uploader);
    void update_instances(render_scene_context& context, gpu_buffer_uploader& uploader);
    void update_batch(render_scene_context& context, gpu_buffer_uploader& uploader);

    gpu_dense_array<mesh_data> m_meshes;
    std::vector<render_id> m_matrix_dirty_meshes;

    gpu_dense_array<instance_data> m_instances;

    gpu_sparse_array<batch_data> m_batches;
    std::unordered_map<render_id, render_id> m_pipeline_to_batch;

    std::vector<std::uint32_t> m_resolve_pipeline_reference_counts;
    std::vector<std::uint32_t> m_shading_model_reference_counts;

    std::vector<material_snapshot> m_materials;
    std::unordered_map<render_id, std::vector<render_id>> m_material_to_instances;

    std::vector<geometry_snapshot> m_geometries;

    std::uint32_t m_material_path_mask{0};

    struct draw_call_info
    {
        std::uint32_t draw_call_capacity;
        std::uint32_t draw_call_count;

        std::array<std::uint32_t, 6> shadow_batch_instance_counts;
    };
    draw_call_info m_draw_call_info;

    enum dirty_flag
    {
        DIRTY_FLAG_BATCH = 1 << 0,
    };
    using dirty_flags = std::uint32_t;

    dirty_flags m_dirty_flags{0};

    std::vector<sphere3f> m_invalidation_bounds;
};
} // namespace violet