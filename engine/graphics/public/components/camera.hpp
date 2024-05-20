#pragma once

#include "graphics/render_graph/render_graph.hpp"
#include "math/math.hpp"
#include <unordered_map>

namespace violet
{
enum camera_state
{
    CAMERA_STATE_DISABLE,
    CAMERA_STATE_ENABLE,
    CAMERA_STATE_RENDER_ONCE
};

class render_device;
class camera
{
public:
    camera(render_device* device);
    camera(const camera&) = delete;
    camera(camera&&) = default;
    ~camera();

    void set_state(camera_state state) noexcept { m_state = state; }
    camera_state get_state() const noexcept { return m_state; }

    void set_priority(float priority) noexcept { m_priority = priority; }
    float get_priority() const noexcept { return m_priority; }

    void set_perspective(float fov, float near_z, float far_z);
    void set_position(const float3& position);
    void set_view(const float4x4& view);
    void set_skybox(rhi_texture* texture, rhi_sampler* sampler);

    rhi_scissor_rect get_scissor() const noexcept { return m_scissor; }
    rhi_viewport get_viewport() const noexcept { return m_viewport; }

    rhi_parameter* get_parameter() const noexcept { return m_parameter.get(); }

    void resize(std::uint32_t width, std::uint32_t height);

    void set_render_graph(render_graph* render_graph) noexcept;
    render_graph* get_render_graph() const noexcept { return m_render_graph; }
    rdg_context* get_render_context() const noexcept { return m_render_context.get(); }

    void set_render_texture(std::string_view name, rhi_texture* texture);
    void set_render_texture(std::string_view name, rhi_swapchain* swapchain);

    const std::vector<std::pair<rhi_swapchain*, std::size_t>>& get_swapchains() const noexcept
    {
        return m_swapchains;
    }

    camera& operator=(const camera&) = delete;
    camera& operator=(camera&&) = default;

private:
    struct parameter_data
    {
        float4x4 view;
        float4x4 projection;
        float4x4 view_projection;
        float3 positon;
        std::uint32_t padding;
    };

    struct perspective_data
    {
        float fov;
        float near_z;
        float far_z;
    };

    void update_projection();
    void update_parameter();

    camera_state m_state;
    float m_priority;

    perspective_data m_perspective;

    parameter_data m_parameter_data;
    rhi_ptr<rhi_parameter> m_parameter;

    rhi_scissor_rect m_scissor;
    rhi_viewport m_viewport;

    render_graph* m_render_graph;
    std::unique_ptr<rdg_context> m_render_context;

    std::vector<std::pair<rhi_swapchain*, std::size_t>> m_swapchains;
};
} // namespace violet