#pragma once

#include "graphics/gpu_array.hpp"
#include "graphics/render_scene/render_scene_module.hpp"
#include "math/box.hpp"

namespace violet
{
class render_scene_sdf : public render_scene_module
{
public:
    struct clipmap
    {
        rhi_buffer* state;

        rhi_texture* page_table;
        rhi_texture* page_atlas;

        rhi_buffer* free_pages;

        std::uint32_t page_atlas_capacity;

        render_id level_offset;

        bool need_clear;
    };

    render_scene_sdf();

    render_id add_mesh();
    void set_mesh_distance_field(render_id mesh_sdf_id, render_id distance_field_id);
    void set_mesh_matrix(render_id mesh_sdf_id, const mat4f& matrix_m, const vec3f& scale);
    void remove_mesh(render_id mesh_sdf_id);

    void update(render_scene_context& context, gpu_buffer_uploader& uploader) override;

    void reset() override;

    rhi_buffer* get_mesh_buffer() const
    {
        return m_meshes.get_buffer()->get_rhi();
    }

    std::uint32_t get_mesh_count() const
    {
        return m_meshes.get_size();
    }

    rhi_buffer* get_invalidation_regions_buffer() const
    {
        return m_invalidation_regions.get_buffer()->get_rhi();
    }

    std::uint32_t get_invalidation_region_count() const
    {
        return m_invalidation_regions.get_size();
    }

    clipmap get_clipmap(render_id camera_id) const
    {
        const auto& clipmap = m_clipmaps[camera_id];
        return {
            .state = clipmap.state.get(),
            .page_table = clipmap.page_table.get(),
            .page_atlas = clipmap.page_atlas.get(),
            .free_pages = clipmap.free_pages.get(),
            .page_atlas_capacity = clipmap.page_atlas_capacity,
            .level_offset = clipmap.level_offset,
            .need_clear = clipmap.need_clear,
        };
    }

    rhi_buffer* get_clipmap_levels_buffer() const
    {
        return m_clipmap_levels.get_buffer()->get_rhi();
    }

private:
    struct clipmap_data
    {
        render_id camera_id;
        render_id level_offset;

        rhi_ptr<rhi_buffer> state;

        rhi_ptr<rhi_texture> page_table;
        rhi_ptr<rhi_texture> page_atlas;

        rhi_ptr<rhi_buffer> free_pages;

        std::uint32_t page_atlas_capacity;

        bool need_clear;
    };

    struct clipmap_level_data
    {
        struct gpu_type
        {
            vec3i position;
            float extent;
            float max_distance;
            std::uint32_t padding0;
            std::uint32_t padding1;
            std::uint32_t padding2;
        };

        vec3i coord;
        std::uint32_t level;
    };

    struct mesh_data
    {
        struct gpu_type
        {
            mat4f volume_to_world;
            mat4f world_to_volume;
            vec3f volume_bounds_min;
            std::uint32_t distance_field_id;
            vec3f volume_bounds_max;
            std::uint32_t padding0;
        };

        std::uint32_t distance_field_id;

        mat4f matrix_m;
        float scale;
    };

    struct invalidation_region
    {
        struct gpu_type
        {
            vec3f bounding_box_min;
            std::uint32_t padding0;
            vec3f bounding_box_max;
            std::uint32_t padding1;
        };

        box3f bounding_box;
    };

    void deallocate_clipmaps(render_scene_context& context);
    void allocate_clipmaps(render_scene_context& context);
    void update_clipmap(render_scene_context& context, gpu_buffer_uploader& uploader);

    void update_meshes(gpu_buffer_uploader& uploader);
    void update_invalidation_regions(gpu_buffer_uploader& uploader);

    std::vector<clipmap_data> m_clipmaps;
    gpu_block_sparse_array<clipmap_level_data> m_clipmap_levels;

    gpu_dense_array<mesh_data> m_meshes;
    gpu_append_array<invalidation_region> m_invalidation_regions;
};
} // namespace violet