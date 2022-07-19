#pragma once

#include "ecs/entity.hpp"
#include "mmd_pipeline.hpp"
#include "physics_interface.hpp"
#include "pmx_loader.hpp"
#include "vmd_loader.hpp"
#include <map>

namespace ash::sample::mmd
{
class mmd_loader
{
public:
    mmd_loader();

    void initialize();
    bool load(
        ecs::entity entity,
        std::string_view pmx,
        std::string_view vmd,
        graphics::render_pipeline* render_pipeline,
        graphics::skin_pipeline* skin_pipeline);

    bool load_pmx(std::string_view pmx);
    bool load_vmd(std::string_view vmd);

private:
    void load_hierarchy(ecs::entity entity, const pmx_loader& loader);
    void load_mesh(
        ecs::entity entity,
        const pmx_loader& loader,
        graphics::skin_pipeline* skin_pipeline);
    void load_material(
        ecs::entity entity,
        const pmx_loader& loader,
        graphics::render_pipeline* render_pipeline);
    void load_physics(ecs::entity entity, const pmx_loader& loader);
    void load_ik(ecs::entity entity, const pmx_loader& loader);
    void load_morph(ecs::entity entity, const pmx_loader& pmx_loader, const vmd_loader& vmd_loader);
    void load_animation(
        ecs::entity entity,
        const pmx_loader& pmx_loader,
        const vmd_loader& vmd_loader);

    std::vector<std::unique_ptr<graphics::resource_interface>> m_internal_toon;

    std::map<std::string, pmx_loader> m_pmx;
    std::map<std::string, vmd_loader> m_vmd;
};
} // namespace ash::sample::mmd