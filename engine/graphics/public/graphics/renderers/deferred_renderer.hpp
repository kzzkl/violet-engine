#pragma once

#include "graphics/renderer.hpp"

namespace violet
{
class deferred_renderer : public renderer
{
public:
    void render(render_graph& graph, const render_context& context) override;

    void set_taa(bool enable) noexcept
    {
        m_enable_taa = enable;
    }

protected:
    // For ImGUI.
    rdg_texture* get_render_target() const noexcept
    {
        return m_render_target;
    }

private:
    void add_cull_pass(render_graph& graph, const render_context& context);
    void add_mesh_pass(render_graph& graph, const render_context& context);
    void add_lighting_pass(render_graph& graph, const render_context& context);
    void add_skybox_pass(render_graph& graph, const render_context& context);
    void add_motion_vector_pass(render_graph& graph, const render_context& context);
    void add_taa_pass(render_graph& graph, const render_context& context);
    void add_tone_mapping_pass(render_graph& graph);
    void add_present_pass(render_graph& graph, const render_context& context);

    rhi_texture_extent m_render_extent;

    rdg_texture* m_render_target{nullptr};
    rdg_texture* m_depth_buffer{nullptr};

    rdg_texture* m_gbuffer_albedo;
    rdg_texture* m_gbuffer_material; // roughness, metallic
    rdg_texture* m_gbuffer_normal;   // octahedron normal
    rdg_texture* m_gbuffer_emissive;
    rdg_texture* m_motion_vectors;

    rdg_buffer* m_command_buffer{nullptr};
    rdg_buffer* m_count_buffer{nullptr};

    bool m_enable_taa{true};
};
} // namespace violet