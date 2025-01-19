#pragma once

#include "core/engine.hpp"
#include "mmd_loader.hpp"
#include "mmd_renderer.hpp"

namespace violet
{
class mmd_debug;
class mmd_viewer : public system
{
public:
    mmd_viewer();
    virtual ~mmd_viewer();

    void install(application& app) override;
    bool initialize(const dictionary& config) override;

private:
    void initialize_render();
    void initialize_scene();

    void tick();
    void resize();

    std::unique_ptr<mmd_renderer> m_renderer;
    rhi_ptr<rhi_swapchain> m_swapchain;

    entity m_camera;
    entity m_light;

    mmd_loader::scene_data m_model_data;

    std::string m_pmx_path;
    std::string m_vmd_path;

    std::vector<std::unique_ptr<texture_2d>> m_internal_toons;

    application* m_app{nullptr};
};
} // namespace violet