#pragma once

#include "context.hpp"
#include "graphics.hpp"
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

    void reset(ecs::entity entity);
    void initialize_pose(ecs::entity entity);

private:
    std::map<std::string, mmd_resource> m_resources;
    ash::ecs::view<mmd_skeleton>* m_skeleton_view;
    std::unique_ptr<mmd_loader> m_loader;
};
} // namespace ash::sample::mmd