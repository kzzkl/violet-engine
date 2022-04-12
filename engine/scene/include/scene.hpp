#pragma once

#include "context.hpp"
#include "transform.hpp"
#include "view.hpp"
#include <memory>

namespace ash::scene
{
class scene : public ash::core::system_base
{
public:
    scene();
    scene(const scene&) = delete;

    virtual bool initialize(const dictionary& config) override;

    void sync_local();
    void sync_world();

    scene& operator=(const scene&) = delete;

    void reset_sync_counter();

    void link(ecs::entity entity);
    void link(ecs::entity child_entity, ecs::entity parent_entity);
    void unlink(ecs::entity entity);

private:
    std::queue<ecs::entity> find_dirty_node() const;

    ash::ecs::view<transform>* m_view;
    ash::ecs::entity m_root;
};
} // namespace ash::scene