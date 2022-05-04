#pragma once

#include "camera.hpp"
#include "context.hpp"
#include "graphics_config.hpp"
#include "graphics_debug.hpp"
#include "graphics_interface_helper.hpp"
#include "graphics_plugin.hpp"
#include "render_parameter.hpp"
#include "render_pipeline.hpp"
#include "transform.hpp"
#include "type_trait.hpp"
#include "view.hpp"
#include "visual.hpp"
#include <set>

namespace ash::graphics
{
class graphics : public ash::core::system_base
{
public:
    template <typename T>
    using interface_map = std::unordered_map<std::string, std::unique_ptr<T>>;

public:
    graphics() noexcept;

    virtual bool initialize(const dictionary& config) override;

    void render(ecs::entity camera_entity);

    void begin_frame();
    void end_frame();

    template <typename T, typename... Args>
    std::unique_ptr<T> make_render_pass(technique_info& info, Args&&... args)
    {
        auto interface = make_technique_interface(info);
        ASH_ASSERT(interface);

        return std::make_unique<T>(interface, std::forward<Args>(args)...);
    }

    void make_render_parameter_layout(std::string_view name, pass_parameter_layout_info& info);
    std::unique_ptr<render_parameter> make_render_parameter(std::string_view name);

    std::unique_ptr<render_target_set_interface> make_render_target_set(
        render_target_set_info& info);

    template <typename Vertex>
    std::unique_ptr<resource> make_vertex_buffer(
        const Vertex* data,
        std::size_t size,
        bool dynamic = false)
    {
        auto& factory = m_plugin.factory();

        vertex_buffer_desc desc;
        if (dynamic)
            desc = {nullptr, sizeof(Vertex), size, dynamic};
        else
            desc = {data, sizeof(Vertex), size, dynamic};
        return std::unique_ptr<resource>(factory.make_vertex_buffer(desc));
    }

    template <typename Index>
    std::unique_ptr<resource> make_index_buffer(
        const Index* data,
        std::size_t size,
        bool dynamic = false)
    {
        auto& factory = m_plugin.factory();
        index_buffer_desc desc = {data, sizeof(Index), size, dynamic};
        return std::unique_ptr<resource>(factory.make_index_buffer(desc));
    }

    std::unique_ptr<resource> make_texture(
        const std::uint8_t* data,
        std::uint32_t width,
        std::uint32_t height)
    {
        auto& factory = m_plugin.factory();
        return std::unique_ptr<resource>(factory.make_texture(data, width, height));
    }

    std::unique_ptr<resource> make_render_target(
        std::uint32_t width,
        std::uint32_t height,
        std::size_t multiple_sampling = 1)
    {
        auto& factory = m_plugin.factory();
        return std::unique_ptr<resource>(
            factory.make_render_target(width, height, multiple_sampling));
    }

    std::unique_ptr<resource> make_depth_stencil(
        std::uint32_t width,
        std::uint32_t height,
        std::size_t multiple_sampling = 1)
    {
        auto& factory = m_plugin.factory();
        return std::unique_ptr<resource>(
            factory.make_depth_stencil(width, height, multiple_sampling));
    }

    std::unique_ptr<resource> make_texture(std::string_view file);

    std::vector<resource*> back_buffers() const;

    graphics_debug& debug() { return *m_debug; }

private:
    technique_interface* make_technique_interface(technique_info& info);

    graphics_plugin m_plugin;
    std::unique_ptr<renderer> m_renderer;

    ash::ecs::view<visual>* m_visual_view;
    ash::ecs::view<visual, scene::transform>* m_object_view;
    ash::ecs::view<main_camera, camera, scene::transform>* m_camera_view;

    // ash::ecs::view<scene::transform>* m_tv;

    std::set<technique*> m_techniques;

    interface_map<pass_parameter_layout_interface> m_parameter_layouts;

    graphics_config m_config;
    std::unique_ptr<graphics_debug> m_debug;
};
} // namespace ash::graphics