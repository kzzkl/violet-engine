#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class sdf_pass
{
public:
    enum debug_mode
    {
        DEBUG_MODE_NONE,
        DEBUG_MODE_PAGE,
        DEBUG_MODE_MESH_SDF,
    };

    struct clipmap
    {
        vec3f position;
        float extent;
        float max_distance;
        std::uint32_t padding0;
        std::uint32_t padding1;
        std::uint32_t padding2;
    };

    struct parameter
    {
        rdg_texture* depth_buffer;

        debug_mode debug_mode{DEBUG_MODE_NONE};
        rdg_texture* debug_output{nullptr};
    };

    void add(render_graph& graph, const parameter& parameter);

private:
    void initialize_clipmaps(render_graph& graph);

    void prepare(render_graph& graph);

    void mark_invalidated_page(render_graph& graph);

    void mesh_classify(render_graph& graph);
    void mesh_grid_cull(render_graph& graph);
    void mesh_page_cull(render_graph& graph);

    void allocate_pages(render_graph& graph);
    void update_pages(render_graph& graph);

    void add_debug_pass(render_graph& graph, const parameter& parameter);

    rdg_buffer* m_clipmap_state;

    std::uint32_t m_clipmap_levels;
    std::uint32_t m_clipmap_level_offset;

    rdg_buffer* m_mesh_buffer;
    std::uint32_t m_mesh_count;

    rdg_buffer* m_clipmap_level_meshes;
    rdg_buffer* m_clipmap_level_mesh_counts;

    rdg_buffer* m_invalidated_grids;
    rdg_buffer* m_invalidated_grid_indirect_args;

    rdg_buffer* m_invalidated_pages;
    rdg_buffer* m_invalidated_page_indirect_args;

    rdg_buffer* m_invalidated_grid_meshes;

    rdg_buffer* m_pages_to_allocate;
    rdg_buffer* m_pages_to_allocate_indirect_args;

    rdg_buffer* m_pages_to_update;
    rdg_buffer* m_pages_to_update_indirect_args;

    rdg_texture* m_page_table;
    rdg_texture* m_page_atlas;
    rdg_buffer* m_free_pages;

    std::uint32_t m_page_atlas_capacity;

    std::uint32_t m_distance_field_buffer;
    std::uint32_t m_brick_table;
    std::uint32_t m_brick_atlas;
};
} // namespace violet