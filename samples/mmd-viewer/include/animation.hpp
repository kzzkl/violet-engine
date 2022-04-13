#pragma once

#include "bone.hpp"
#include "context.hpp"
#include "skeleton.hpp"
#include "transform.hpp"
#include "view.hpp"

namespace ash::sample::mmd
{
class animation : public ash::core::system_base
{
public:
    animation() : system_base("animation") {}

    virtual bool initialize(const ash::dictionary& config) override;

    void update();

private:
    ash::ecs::view<skeleton>* m_view;
    ash::ecs::view<bone, scene::transform>* m_bone_view;
};
} // namespace ash::sample::mmd