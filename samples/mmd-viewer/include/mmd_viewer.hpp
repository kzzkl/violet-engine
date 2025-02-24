#pragma once

#include "core/engine.hpp"
#include "graphics/skybox.hpp"
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
    void initialize_scene(
        const std::filesystem::path& pmx_path,
        const std::filesystem::path& vmd_path,
        const std::filesystem::path& skybox_path);

    void override_material(const dictionary& info, const std::filesystem::path& root_path);

    void load_gf2_material(const dictionary& info, const std::filesystem::path& root_path);

    void resize();

    void update_sdf();
    void draw_imgui();

    std::unique_ptr<mmd_renderer> m_renderer;
    rhi_ptr<rhi_swapchain> m_swapchain;

    entity m_camera;
    entity m_light;

    mmd_loader::scene_data m_model;

    entity m_face;
    std::function<void(const vec3f&, const vec3f&)> m_sdf_callback;

    std::unique_ptr<skybox> m_skybox;

    std::vector<std::unique_ptr<texture_2d>> m_internal_toons;

    application* m_app{nullptr};
};
} // namespace violet