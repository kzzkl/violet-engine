#pragma once

#include "core/context.hpp"
#include "graphics/compute_pipeline.hpp"
#include "graphics/graphics_config.hpp"
#include "graphics/graphics_debug.hpp"
#include "graphics/graphics_interface_helper.hpp"
#include "graphics/graphics_plugin.hpp"
#include "graphics/pipeline_parameter.hpp"
#include "graphics/skinned_mesh.hpp"
#include "graphics/visual.hpp"
#include "scene/transform.hpp"
#include "type_trait.hpp"

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
    virtual void shutdown() override;

    void compute(compute_pipeline* pipeline);

    void skin_meshes();
    void render(ecs::entity target_camera = ecs::INVALID_ENTITY);
    void present();

    void game_camera(ecs::entity game_camera) noexcept { m_game_camera = game_camera; }
    ecs::entity game_camera() const noexcept { return m_game_camera; }

    void editor_camera(ecs::entity editor_camera, ecs::entity scene_camera) noexcept
    {
        ASH_ASSERT(editor_camera != ecs::INVALID_ENTITY && scene_camera != ecs::INVALID_ENTITY);

        m_editor_camera = editor_camera;
        m_scene_camera = scene_camera;
    }

    std::unique_ptr<render_pipeline_interface> make_render_pipeline(render_pipeline_info& info);
    std::unique_ptr<compute_pipeline_interface> make_compute_pipeline(compute_pipeline_info& info);

    void make_pipeline_parameter_layout(
        std::string_view name,
        pipeline_parameter_layout_info& info);
    std::unique_ptr<pipeline_parameter> make_pipeline_parameter(std::string_view name);

    template <typename Vertex>
    std::unique_ptr<resource> make_vertex_buffer(
        const Vertex* data,
        std::size_t size,
        vertex_buffer_flags flags = VERTEX_BUFFER_FLAG_NONE,
        bool dynamic = false,
        bool frame_resource = false)
    {
        auto& factory = m_plugin.factory();
        vertex_buffer_desc desc = {
            .vertices = data,
            .vertex_size = sizeof(Vertex),
            .vertex_count = size,
            .flags = flags,
            .dynamic = dynamic,
            .frame_resource = frame_resource};
        return std::unique_ptr<resource>(factory.make_vertex_buffer(desc));
    }

    template <typename Index>
    std::unique_ptr<resource> make_index_buffer(
        const Index* data,
        std::size_t size,
        bool dynamic = false,
        bool frame_resource = false)
    {
        auto& factory = m_plugin.factory();
        index_buffer_desc desc = {
            .indices = data,
            .index_size = sizeof(Index),
            .index_count = size,
            .dynamic = dynamic,
            .frame_resource = frame_resource};
        return std::unique_ptr<resource>(factory.make_index_buffer(desc));
    }

    std::unique_ptr<resource> make_texture(
        const std::uint8_t* data,
        std::uint32_t width,
        std::uint32_t height,
        resource_format format = resource_format::B8G8R8A8_UNORM)
    {
        auto& factory = m_plugin.factory();
        return std::unique_ptr<resource>(factory.make_texture(data, width, height, format));
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

    graphics_debug& debug() { return *m_debug; }

    resource_format back_buffer_format() const { return m_renderer->back_buffer()->format(); }
    resource_extent render_extent() const noexcept;

private:
    void render_main_camera();
    bool editor_mode() const noexcept { return m_editor_camera != ecs::INVALID_ENTITY; }

    graphics_plugin m_plugin;
    std::unique_ptr<renderer> m_renderer;
    std::size_t m_back_buffer_index;

    ecs::entity m_game_camera;
    ecs::entity m_editor_camera;
    ecs::entity m_scene_camera;

    ash::ecs::view<visual>* m_visual_view;
    ash::ecs::view<visual, scene::transform>* m_object_view;
    ash::ecs::view<visual, skinned_mesh>* m_skinned_mesh_view;

    interface_map<pipeline_parameter_layout_interface> m_parameter_layouts;

    graphics_config m_config;
    std::unique_ptr<graphics_debug> m_debug;
};
} // namespace ash::graphics