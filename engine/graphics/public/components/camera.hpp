#pragma once

#include "graphics/render_graph/render_pass.hpp"
#include "math/math.hpp"

namespace violet
{
struct camera_parameter
{
    static constexpr rhi_pipeline_parameter_layout_desc layout = {
        .parameters = {{RHI_PIPELINE_PARAMETER_TYPE_UNIFORM_BUFFER, sizeof(float4x4) * 3}},
        .parameter_count = 1};

    float4x4 view;
    float4x4 projection;
    float4x4 view_projection;
};

class camera
{
public:
    camera();
    camera(const camera&) = delete;
    camera(camera&& other) noexcept;
    ~camera();

    void set_perspective(float fov, float near_z, float far_z);
    void set_view(const float4x4& view);

    rhi_scissor_rect get_scissor() const noexcept { return m_scissor; }
    rhi_viewport get_viewport() const noexcept { return m_viewport; }

    void set_render_pass(render_pass* render_pass);
    render_pass* get_render_pass() const noexcept { return m_render_pass; }

    void set_attachment(std::size_t index, rhi_resource* attachment, bool back_buffer = false);
    void set_back_buffer(rhi_resource* back_buffer);

    rhi_framebuffer* get_framebuffer();

    rhi_pipeline_parameter* get_parameter() const noexcept { return m_parameter; }

    void resize(std::uint32_t width, std::uint32_t height);

    camera& operator=(const camera&) = delete;
    camera& operator=(camera&& other) noexcept;

private:
    struct perspective_data
    {
        float fov;
        float near_z;
        float far_z;
    };

    void update_projection();
    void update_parameter();

    perspective_data m_perspective;

    camera_parameter m_parameter_data;
    rhi_pipeline_parameter* m_parameter;
    render_pass* m_render_pass;

    rhi_scissor_rect m_scissor;
    rhi_viewport m_viewport;

    std::vector<rhi_resource*> m_attachments;
    std::size_t m_back_buffer_index;

    bool m_framebuffer_dirty;
    rhi_framebuffer* m_framebuffer;
    std::unordered_map<std::size_t, rhi_framebuffer*> m_framebuffer_cache;
};
} // namespace violet