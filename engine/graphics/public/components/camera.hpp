#pragma once

#include "graphics/renderer.hpp"
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
    camera();
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

    void resize(std::uint32_t width, std::uint32_t height);

    void set_renderer(renderer* renderer) noexcept;
    renderer* get_renderer() const noexcept { return m_renderer; }

    void set_render_target(std::size_t index, rhi_texture* texture);
    void set_render_target(std::size_t index, rhi_swapchain* swapchain);

    std::vector<rhi_texture*> get_render_targets() const;
    std::vector<rhi_swapchain*> get_swapchains() const;

    rhi_parameter* get_parameter() const noexcept { return m_parameter.get(); }

    camera& operator=(const camera&) = delete;
    camera& operator=(camera&&) = default;

private:
    struct perspective_data
    {
        float fov;
        float near_z;
        float far_z;
    };

    struct render_target
    {
        bool is_swapchain;
        union {
            rhi_swapchain* swapchain;
            rhi_texture* texture;
        };
    };

    void update_projection();

    camera_state m_state;
    float m_priority;

    perspective_data m_perspective;

    float4x4 m_view;
    float4x4 m_projection;

    rhi_ptr<rhi_parameter> m_parameter;

    rhi_scissor_rect m_scissor;
    rhi_viewport m_viewport;

    renderer* m_renderer;

    std::vector<render_target> m_render_targets;
};
} // namespace violet