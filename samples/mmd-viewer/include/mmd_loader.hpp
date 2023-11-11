#pragma once

#include "core/ecs/actor.hpp"
#include "graphics/geometry.hpp"
#include "graphics/render_graph/render_graph.hpp"
#include "physics/physics_interface.hpp"
#include <map>
#include <memory>

namespace violet::sample
{
struct mmd_model
{
    std::vector<rhi_resource*> textures;
    std::vector<material*> materials;

    std::unique_ptr<geometry> geometry;
    std::unique_ptr<actor> model;

    std::vector<std::unique_ptr<actor>> bones;

    std::vector<pei_collision_shape*> collision_shapes;
};

class pmx_mesh;
class mmd_loader
{
public:
    mmd_loader(render_graph* render_graph, rhi_renderer* rhi, pei_plugin* pei);
    ~mmd_loader();

    mmd_model* load(std::string_view path, world& world);

    mmd_model* get_model(std::string_view name) const noexcept
    {
        return m_models.at(name.data()).get();
    }

private:
    void load_bones(mmd_model* model, const pmx_mesh& pmx, world& world);
    void load_physics(mmd_model* model, const pmx_mesh& pmx, world& world);

    std::map<std::string, std::unique_ptr<mmd_model>> m_models;

    std::vector<rhi_resource*> m_internal_toons;

    render_graph* m_render_graph;
    rhi_renderer* m_rhi;
    rhi_sampler* m_sampler;

    pei_plugin* m_pei;
};
} // namespace violet::sample