#pragma once

#include "context.hpp"
#include "scene_exports.hpp"
#include "transform.hpp"
#include "view.hpp"
#include <memory>

namespace ash::scene
{
class SCENE_API scene : public ash::core::submodule
{
public:
    static constexpr uuid id = "88be6763-ea80-417d-acf9-17eaab46a4ea";

public:
    scene();
    scene(const scene&) = delete;

    virtual bool initialize(const dictionary& config) override;

    void sync_local();
    void sync_world();

    scene& operator=(const scene&) = delete;

    void reset_sync_counter();

    void link(transform& node);
    void link(transform& child, transform& parent);
    void unlink(transform& node);

private:
    std::queue<transform_node*> find_dirty_node() const;

    ash::ecs::view<transform>* m_view;
    ash::ecs::read<transform> m_root;
};
} // namespace ash::scene