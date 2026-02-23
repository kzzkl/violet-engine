#pragma once

#include "core/engine.hpp"
#include "graphics/geometry.hpp"
#include "graphics/material.hpp"
#include "graphics/skybox.hpp"

namespace violet
{
class rdg_profiling;
class sample_system : public system
{
public:
    enum load_option
    {
        LOAD_OPTION_NONE = 0,
        LOAD_OPTION_DYNAMIC_MESH = 1 << 0,
        LOAD_OPTION_GENERATE_CLUSTERS = 1 << 1,
    };
    using load_options = std::uint32_t;

    sample_system(std::string_view name);

    void install(application& app) override;
    bool initialize(const dictionary& config) override;

protected:
    entity load_model(std::string_view model_path, load_options options = LOAD_OPTION_NONE);

    entity get_light() const noexcept
    {
        return m_light;
    }

    entity get_camera() const noexcept
    {
        return m_camera;
    }

    void imgui_profiling(rdg_profiling* profiling);

private:
    virtual void tick() {}

    void initialize_render();
    void initialize_scene(std::string_view skybox_path);

    std::unique_ptr<skybox> m_skybox;
    entity m_light;
    entity m_camera;

    std::vector<std::unique_ptr<geometry>> m_geometries;
    std::vector<std::unique_ptr<material>> m_materials;
    std::vector<std::unique_ptr<texture_2d>> m_textures;

    rhi_ptr<rhi_swapchain> m_swapchain;

    application* m_app{nullptr};
};
} // namespace violet