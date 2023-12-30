#pragma once

#include "core/engine_system.hpp"
#include "mmd_loader.hpp"
#include "mmd_render.hpp"
#include "physics/physics_world.hpp"

namespace violet::sample
{
class physics_debug;
class mmd_viewer : public engine_system
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

    std::unique_ptr<mmd_render_graph> m_render_graph;
    rhi_ptr<rhi_image> m_depth_stencil;

    std::unique_ptr<actor> m_camera;
    std::unique_ptr<actor> m_light;

    std::unique_ptr<physics_world> m_physics_world;
    std::unique_ptr<physics_debug> m_physics_debug;

    std::unique_ptr<mmd_loader> m_loader;
    mmd_model* m_model;
};
} // namespace violet::sample