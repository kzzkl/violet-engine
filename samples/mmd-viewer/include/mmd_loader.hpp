#pragma once

#include "ecs/entity.hpp"
#include "graphics/geometry.hpp"
#include "graphics/material.hpp"
#include "mesh_loader.hpp"
#include "physics/physics_context.hpp"
#include <memory>

namespace violet::sample
{
/*struct mmd_model
{
    std::vector<rhi_ptr<rhi_texture>> textures;
    std::vector<std::unique_ptr<material>> materials;

    std::unique_ptr<geometry> geometry;
    entity model;

    std::vector<entity> bones;

    std::vector<phy_ptr<phy_collision_shape>> collision_shapes;
};*/

class pmx;
class vmd;
class mmd_loader : public mesh_loader
{
public:
    mmd_loader(std::string_view pmx, std::string_view vmd);
    ~mmd_loader();

    std::optional<scene_data> load() override;

private:
    void load_mesh(scene_data& scene, const pmx& pmx);
    // void load_bones(mmd_model* model, const pmx& pmx, world& world);
    // void load_morph(mmd_model* model, const pmx& pmx);
    // void load_physics(mmd_model* model, const pmx& pmx, world& world);
    // void load_animation(mmd_model* model, const vmd& vmd, world& world);

    std::string m_pmx_path;
    std::string m_vmd_path;
};
} // namespace violet::sample