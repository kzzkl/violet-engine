#pragma once

#include "core/engine_module.hpp"
#include "mmd_loader.hpp"
#include "mmd_renderer.hpp"
#include "physics/physics_world.hpp"

namespace violet::sample
{
class physics_debug;
class mmd_viewer : public engine_module
{
public:
    mmd_viewer();
    virtual ~mmd_viewer();

    virtual bool initialize(const dictionary& config) override;
    virtual void shutdown() override;

private:
    void initialize_render();

    void tick(float delta);
    void resize(std::uint32_t width, std::uint32_t height);

    std::unique_ptr<mmd_renderer> m_renderer;
    rhi_ptr<rhi_texture> m_depth_stencil;
    rhi_ptr<rhi_swapchain> m_swapchain;

    std::unique_ptr<actor> m_camera;
    std::unique_ptr<actor> m_light;

    std::unique_ptr<physics_world> m_physics_world;
    std::unique_ptr<physics_debug> m_physics_debug;

    std::unique_ptr<mmd_loader> m_loader;
    mmd_model* m_model;
};
} // namespace violet::sample