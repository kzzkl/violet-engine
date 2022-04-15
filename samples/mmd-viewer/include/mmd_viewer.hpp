#pragma once

#include "context.hpp"
#include "graphics.hpp"
#include "mmd_ik.hpp"
#include "mmd_loader.hpp"
#include "physics.hpp"

namespace ash::sample::mmd
{
class mmd_viewer : public ash::core::system_base
{
public:
    mmd_viewer() : system_base("mmd_viewer") {}

    virtual bool initialize(const dictionary& config) override;
    ash::ecs::entity load_mmd(std::string_view name, std::string_view pmx, std::string_view vmd);

    void update();
    void update_animation(ecs::entity entity, float t, float weight);
    void update_node(ecs::entity entity, bool after_physics);

    void reset(ecs::entity entity);

private:
    void update_inherit(ecs::entity entity);
    void update_ik(ecs::entity entity);

    void sync_node(bool after_physics);

    std::map<std::string, mmd_resource> m_resources;

    ash::ecs::view<mmd_skeleton>* m_skeleton_view;
    ash::ecs::view<mmd_bone, scene::transform>* m_bone_view;

    std::unique_ptr<mmd_loader> m_loader;
    std::unique_ptr<mmd_ik_solver> m_ik_solver;
};
} // namespace ash::sample::mmd