#pragma once

#include "context.hpp"
#include "link.hpp"
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

    ecs::entity root() const noexcept { return m_root; }

    void sync_local();
    void sync_local(ecs::entity root);
    void sync_world();
    void sync_world(ecs::entity root);

    scene& operator=(const scene&) = delete;

    void reset_sync_counter();

private:
    void on_entity_link(ecs::entity entity, core::link& link);
    void on_entity_unlink(ecs::entity entity, core::link& link);

    ash::ecs::view<transform>* m_view;
    ash::ecs::entity m_root;
};
} // namespace ash::scene