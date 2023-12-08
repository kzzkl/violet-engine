#pragma once

#include "core/ecs/actor.hpp"
#include "graphics/geometry.hpp"
#include "graphics/render_graph/render_graph.hpp"
#include "physics/physics_context.hpp"
#include <map>
#include <memory>

namespace violet::sample
{
struct mmd_model
{
    std::vector<rhi_ptr<rhi_resource>> textures;
    std::vector<material*> materials;

    std::unique_ptr<geometry> geometry;
    std::unique_ptr<actor> model;

    std::vector<std::unique_ptr<actor>> bones;

    std::vector<pei_ptr<pei_collision_shape>> collision_shapes;
};

class pmx;
class vmd;
class mmd_loader
{
public:
    mmd_loader(render_graph* render_graph, renderer* renderer, physics_context* physics_context);
    ~mmd_loader();

    mmd_model* load(std::string_view pmx_path, std::string_view vmd_path, world& world);

    mmd_model* get_model(std::string_view name) const noexcept
    {
        return m_models.at(name.data()).get();
    }

private:
    void load_mesh(mmd_model* model, const pmx& pmx, world& world);
    void load_bones(mmd_model* model, const pmx& pmx, world& world);
    void load_morph(mmd_model* model, const pmx& pmx);
    void load_physics(mmd_model* model, const pmx& pmx, world& world);
    void load_animation(mmd_model* model, const vmd& vmd, world& world);

    std::map<std::string, std::unique_ptr<mmd_model>> m_models;

    std::vector<rhi_ptr<rhi_resource>> m_internal_toons;

    render_graph* m_render_graph;
    renderer* m_renderer;
    rhi_ptr<rhi_sampler> m_sampler;

    physics_context* m_physics_context;
};
} // namespace violet::sample