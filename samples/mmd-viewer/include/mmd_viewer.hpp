#pragma once

#include "core/engine_system.hpp"
#include "mmd_loader.hpp"
#include "mmd_renderer.hpp"

namespace violet::sample
{
class physics_debug;
class mmd_viewer : public engine_system
{
public:
    mmd_viewer();
    virtual ~mmd_viewer();

    bool initialize(const dictionary& config) override;

private:
    void initialize_render();
    void initialize_scene();

    void tick(float delta);
    void resize();

    std::unique_ptr<renderer> m_renderer;
    rhi_ptr<rhi_swapchain> m_swapchain;

    entity m_camera;
    entity m_light;

    mesh_loader::scene_data m_model_data;

    std::string m_pmx_path;
    std::string m_vmd_path;

    std::unique_ptr<material> m_material;

    // std::unique_ptr<physics_world> m_physics_world;
    // std::unique_ptr<physics_debug> m_physics_debug;
};
} // namespace violet::sample