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
class graphics : public core::system_base
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

    std::unique_ptr<render_pass_interface> make_render_pass(render_pass_info& info);

    void make_pipeline_layout(std::string_view name, pipeline_layout_info& info);
    std::unique_ptr<pipeline_parameter> make_pipeline_parameter(std::string_view name);

    // std::unique_ptr<attachment_set_interface> make_attachment_set(attachment_set_info& info);

    template <typename Vertex>
    std::unique_ptr<resource> make_vertex_buffer(
        const Vertex* data,
        std::size_t size,
        bool dynamic = false)
    {
        auto& factory = m_plugin.factory();
        vertex_buffer_desc desc = {data, sizeof(Vertex), size, dynamic};
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

    std::unique_ptr<resource> make_render_target(const render_target_desc& desc)
    {
        auto& factory = m_plugin.factory();
        return std::unique_ptr<resource>(factory.make_render_target(desc));
    }

    std::unique_ptr<resource> make_depth_stencil_buffer(const depth_stencil_buffer_desc& desc)
    {
        auto& factory = m_plugin.factory();
        return std::unique_ptr<resource>(factory.make_depth_stencil_buffer(desc));
    }

    std::unique_ptr<resource> make_texture(std::string_view file);

    std::vector<resource*> back_buffers() const;

    graphics_debug& debug() { return *m_debug; }

private:
    graphics_plugin m_plugin;
    std::unique_ptr<renderer> m_renderer;
    std::size_t m_back_buffer_index;

    ash::ecs::view<visual>* m_visual_view;
    ash::ecs::view<visual, scene::transform>* m_object_view;
    ash::ecs::view<main_camera, camera, scene::transform>* m_camera_view;

    // ash::ecs::view<scene::transform>* m_tv;

    std::set<render_pass*> m_render_passes;

    interface_map<pipeline_layout_interface> m_parameter_layouts;

    graphics_config m_config;
    std::unique_ptr<graphics_debug> m_debug;
};
} // namespace ash::graphics