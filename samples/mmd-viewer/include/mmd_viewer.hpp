#pragma once

#include "core/context.hpp"
#include "graphics/graphics.hpp"
#include "mmd_component.hpp"
#include "mmd_loader.hpp"
#include "mmd_pipeline.hpp"
#include "physics/physics.hpp"

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

private:
    std::map<std::string, mmd_resource> m_resources;
    ash::ecs::view<mmd_skeleton, graphics::skinned_mesh>* m_skeleton_view;
    std::unique_ptr<mmd_loader> m_loader;

    std::unique_ptr<mmd_render_pipeline> m_render_pipeline;
    std::unique_ptr<mmd_skin_pipeline> m_skin_pipeline;
};
} // namespace ash::sample::mmd