#pragma once

#include "graphics/render_graph/render_pass.hpp"
#include "math/math.hpp"
#include <unordered_map>

namespace violet
{
struct camera_parameter
{
    float4x4 view;
    float4x4 projection;
    float4x4 view_projection;
    float3 positon;
    std::uint32_t padding;
};

class camera
{
public:
    camera(renderer* renderer);
    camera(const camera&) = delete;
    camera(camera&&) = default;
    ~camera();

    void set_perspective(float fov, float near_z, float far_z);
    void set_position(const float3& position);
    void set_view(const float4x4& view);
    void set_skybox(rhi_resource* texture, rhi_sampler* sampler);

    rhi_scissor_rect get_scissor() const noexcept { return m_scissor; }
    rhi_viewport get_viewport() const noexcept { return m_viewport; }

    void set_render_pass(render_pass* render_pass);
    render_pass* get_render_pass() const noexcept { return m_render_pass; }

    void set_attachment(std::size_t index, rhi_resource* attachment, bool back_buffer = false);
    void set_back_buffer(rhi_resource* back_buffer);

    rhi_framebuffer* get_framebuffer();

    rhi_parameter* get_parameter() const noexcept { return m_parameter.get(); }

    void resize(std::uint32_t width, std::uint32_t height);

    camera& operator=(const camera&) = delete;
    camera& operator=(camera&&) = default;

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
    rhi_ptr<rhi_parameter> m_parameter;
    render_pass* m_render_pass;

    rhi_scissor_rect m_scissor;
    rhi_viewport m_viewport;

    std::vector<rhi_resource*> m_attachments;
    std::size_t m_back_buffer_index;

    bool m_framebuffer_dirty;
    rhi_framebuffer* m_framebuffer;
    std::unordered_map<std::size_t, rhi_ptr<rhi_framebuffer>> m_framebuffer_cache;

    renderer* m_renderer;
};
} // namespace violet