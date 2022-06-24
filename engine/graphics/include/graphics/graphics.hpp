#pragma once

#include "core/context.hpp"
#include "graphics/compute_pipeline.hpp"
#include "graphics/graphics_debug.hpp"
#include "graphics/light.hpp"
#include "graphics/mesh_render.hpp"
#include "graphics/skinned_mesh.hpp"
#include "scene/transform.hpp"
#include "type_trait.hpp"

namespace ash::graphics
{
class graphics : public core::system_base
{
public:
    graphics() noexcept;

    virtual bool initialize(const dictionary& config) override;
    virtual void shutdown() override;

    void compute(compute_pipeline* pipeline);

    void skin_meshes();
    void render(ecs::entity camera_entity);

    void game_camera(ecs::entity game_camera) noexcept { m_game_camera = game_camera; }
    ecs::entity game_camera() const noexcept { return m_game_camera; }

    void editor_camera(ecs::entity editor_camera, ecs::entity scene_camera) noexcept
    {
        ASH_ASSERT(editor_camera != ecs::INVALID_ENTITY && scene_camera != ecs::INVALID_ENTITY);

        m_editor_camera = editor_camera;
        m_scene_camera = scene_camera;
    }

    graphics_debug& debug() { return *m_debug; }

    resource_extent render_extent() const noexcept;

private:
    void render();
    void render_camera(ecs::entity camera_entity);
    void present();

    bool is_editor_mode() const noexcept { return m_editor_camera != ecs::INVALID_ENTITY; }

    std::size_t m_back_buffer_index;

    ecs::entity m_game_camera;
    ecs::entity m_editor_camera;
    ecs::entity m_scene_camera;

    std::queue<ecs::entity> m_render_queue;

    std::unique_ptr<pipeline_parameter_interface> m_light_parameter;

    ecs::view<mesh_render>* m_render_view;
    ecs::view<mesh_render, scene::transform>* m_object_view;
    ecs::view<mesh_render, skinned_mesh>* m_skinned_mesh_view;

    ecs::view<point_light>* m_point_light_view;

    std::unique_ptr<graphics_debug> m_debug;
};
} // namespace ash::graphics