#pragma once

#include "graphics/render_interface.hpp"
#include <vector>

namespace violet
{
class render_scene;

struct render_camera
{
    rhi_parameter* camera_parameter;
    std::vector<rhi_texture*> render_targets;

    rhi_viewport viewport;
    std::vector<rhi_scissor_rect> scissor_rects;
};

class render_context
{
public:
    render_context(const render_scene& scene, const render_camera& camera);

    std::size_t get_mesh_count() const noexcept;
    std::size_t get_mesh_capacity() const noexcept;

    std::size_t get_instance_count() const noexcept;
    std::size_t get_instance_capacity() const noexcept;

    std::size_t get_group_count() const noexcept;
    std::size_t get_group_capacity() const noexcept;

    rhi_parameter* get_scene_parameter() const noexcept;

    rhi_parameter* get_camera_parameter() const noexcept
    {
        return m_camera.camera_parameter;
    }

    rhi_texture* get_render_target(std::size_t index) const noexcept
    {
        return m_camera.render_targets[index];
    }

    const rhi_viewport& get_viewport() const noexcept
    {
        return m_camera.viewport;
    }

    const std::vector<rhi_scissor_rect>& get_scissor_rects() const noexcept
    {
        return m_camera.scissor_rects;
    }

    bool has_skybox() const noexcept;

    const render_scene& get_scene() const noexcept
    {
        return m_scene;
    }

private:
    const render_scene& m_scene;
    const render_camera& m_camera;
};
} // namespace violet