#pragma once

#include "graphics/renderer.hpp"

namespace violet
{
class deferred_renderer : public renderer
{
public:
    deferred_renderer();

protected:
    void on_render(render_graph& graph) override;

    // For ImGUI.
    rdg_texture* get_render_target() const noexcept
    {
        return m_render_target;
    }

private:
    void add_cull_pass(render_graph& graph);
    void add_mesh_pass(render_graph& graph);
    void add_hzb_pass(render_graph& graph);
    void add_gtao_pass(render_graph& graph);
    void add_lighting_pass(render_graph& graph);
    void add_skybox_pass(render_graph& graph);
    void add_motion_vector_pass(render_graph& graph);
    void add_taa_pass(render_graph& graph);
    void add_tone_mapping_pass(render_graph& graph);
    void add_present_pass(render_graph& graph);

    rhi_texture_extent m_render_extent;

    rdg_texture* m_render_target{nullptr};
    rdg_texture* m_depth_buffer{nullptr};
    rdg_texture* m_hzb{nullptr};

    rdg_texture* m_gbuffer_albedo{nullptr};
    rdg_texture* m_gbuffer_material{nullptr}; // roughness, metallic
    rdg_texture* m_gbuffer_normal{nullptr};   // octahedron normal
    rdg_texture* m_gbuffer_emissive{nullptr};

    rdg_texture* m_ao_buffer{nullptr};
    rdg_texture* m_motion_vectors{nullptr};

    rdg_buffer* m_command_buffer{nullptr};
    rdg_buffer* m_count_buffer{nullptr};
};
} // namespace violet