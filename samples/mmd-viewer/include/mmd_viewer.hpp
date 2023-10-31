#pragma once

#include "core/engine_system.hpp"
#include "core/node/node.hpp"
#include "graphics/geometry.hpp"
#include "mmd_render.hpp"

namespace violet::sample
{
class mmd_viewer : public engine_system
{
public:
    mmd_viewer();
    virtual ~mmd_viewer();

    virtual bool initialize(const dictionary& config) override;
    virtual void shutdown() override;

private:
    struct mmd_model
    {
        std::vector<rhi_resource*> textures;
        std::vector<material*> materials;

        std::unique_ptr<geometry> geometry;
        std::unique_ptr<node> model;
    };

    void initialize_render();
    void load_model(std::string_view path);

    void tick(float delta);
    void resize(std::uint32_t width, std::uint32_t height);

    std::unique_ptr<mmd_render_graph> m_render_graph;
    rhi_resource* m_depth_stencil;
    rhi_sampler* m_sampler;

    std::vector<rhi_resource*> m_internal_toons;

    std::map<std::string, mmd_model> m_models;
    std::unique_ptr<node> m_camera;
};
} // namespace violet::sample