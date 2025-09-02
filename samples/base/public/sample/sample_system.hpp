#pragma once

#include "core/engine.hpp"
#include "graphics/geometry.hpp"
#include "graphics/material.hpp"
#include "graphics/skybox.hpp"

namespace violet
{
class sample_system : public system
{
public:
    sample_system(std::string_view name);

    void install(application& app) override;
    bool initialize(const dictionary& config) override;

protected:
    entity load_model(std::string_view model_path);

    entity get_light() const noexcept
    {
        return m_light;
    }

    entity get_camera() const noexcept
    {
        return m_camera;
    }

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