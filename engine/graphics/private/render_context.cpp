#include "graphics/render_context.hpp"
#include "graphics/render_scene.hpp"

namespace violet
{
render_context::render_context(const render_scene& scene, const render_camera& camera)
    : m_scene(scene),
      m_camera(camera)
{
}

std::size_t render_context::get_mesh_count() const noexcept
{
    return m_scene.get_mesh_count();
}

std::size_t render_context::get_mesh_capacity() const noexcept
{
    return m_scene.get_mesh_capacity();
}

std::size_t render_context::get_instance_count() const noexcept
{
    return m_scene.get_instance_count();
}

std::size_t render_context::get_instance_capacity() const noexcept
{
    return m_scene.get_instance_capacity();
}

std::size_t render_context::get_group_count() const noexcept
{
    return m_scene.get_group_count();
}

std::size_t render_context::get_group_capacity() const noexcept
{
    return m_scene.get_group_capacity();
}

rhi_parameter* render_context::get_scene_parameter() const noexcept
{
    return m_scene.get_scene_parameter();
}

bool render_context::has_skybox() const noexcept
{
    return m_scene.has_skybox();
}
} // namespace violet