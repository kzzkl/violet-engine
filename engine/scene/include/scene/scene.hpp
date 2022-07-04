#pragma once

#include "core/context.hpp"
#include "core/link.hpp"
#include "ecs/world.hpp"
#include "scene/bvh_tree.hpp"
#include "scene/transform.hpp"
#include <memory>

namespace ash::scene
{
class scene : public ash::core::system_base
{
public:
    scene();
    scene(const scene&) = delete;

    virtual bool initialize(const dictionary& config) override;

    ecs::entity root() const noexcept { return m_root; }

    void sync_local();
    void sync_local(ecs::entity root);
    void sync_world();
    void sync_world(ecs::entity root);

    void frustum_culling(const std::vector<math::float4>& frustum);

private:
    void on_entity_link(ecs::entity entity, core::link& link);
    void on_entity_unlink(ecs::entity entity, core::link& link);

    ash::ecs::view<transform>* m_view;
    ash::ecs::view<bounding_box>* m_bounding_box_view;
    ash::ecs::entity m_root;

    bvh_tree m_static_bvh;
    bvh_tree m_dynamic_bvh;
};
} // namespace ash::scene