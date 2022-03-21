#pragma once

#include "context.hpp"
#include "scene_exports.hpp"
#include "scene_node.hpp"
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

    inline scene_node* root_node() const noexcept { return m_root_node.get(); }

    scene& operator=(const scene&) = delete;

private:
    void update_hierarchy();
    void update_to_world();

    std::unique_ptr<scene_node> m_root_node;

    ash::ecs::view<transform>* m_view;
};
} // namespace ash::scene