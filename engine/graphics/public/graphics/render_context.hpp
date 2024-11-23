#pragma once

#include "graphics/render_device.hpp"
#include "graphics/render_scene.hpp"

namespace violet
{
class material_manager;
class geometry_manager;

class render_context
{
public:
    ~render_context();

    static render_context& instance();

    render_scene* get_scene(std::uint32_t scene_id);

    bool update();
    void record(rhi_command* command);

    material_manager* get_material_manager() const noexcept
    {
        return m_material_manager.get();
    }

    geometry_manager* get_geometry_manager() const noexcept
    {
        return m_geometry_manager.get();
    }

    rhi_parameter* get_global_parameter() const noexcept
    {
        return m_global_parameter.get();
    }

    void reset();

private:
    render_context();
    render_context(const render_context&) = delete;

    render_context& operator=(const render_context&) = delete;

    std::unique_ptr<material_manager> m_material_manager;
    std::unique_ptr<geometry_manager> m_geometry_manager;

    std::vector<std::unique_ptr<render_scene>> m_scenes;

    rhi_ptr<rhi_parameter> m_global_parameter;
};
} // namespace violet