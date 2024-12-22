#pragma once

#include "ecs/entity.hpp"
#include "ecs/world.hpp"
#include "graphics/geometry.hpp"
#include "graphics/material.hpp"
#include "pmx.hpp"
#include "vmd.hpp"
#include <memory>
#include <optional>

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

class vmd;
class mmd_loader
{
public:
    struct scene_data
    {
        std::vector<rhi_ptr<rhi_texture>> textures;
        std::vector<std::unique_ptr<geometry>> geometries;
        std::vector<std::unique_ptr<material>> materials;

        entity root;
    };

    mmd_loader(const std::vector<rhi_texture*>& internal_toons);
    ~mmd_loader();

    std::optional<scene_data> load(std::string_view pmx, std::string_view vmd, world& world);

private:
    void load_mesh(scene_data& scene, world& world);
    void load_bone(world& world);
    void load_physics(world& world);
    void load_animation(world& world);
    // void load_morph(mmd_model* model, const pmx& pmx);

    pmx m_pmx;
    vmd m_vmd;

    entity m_root;
    std::vector<entity> m_bones;
    std::vector<mat4f> m_bone_initial_transforms;

    std::vector<rhi_texture*> m_internal_toons;
};
} // namespace violet::sample