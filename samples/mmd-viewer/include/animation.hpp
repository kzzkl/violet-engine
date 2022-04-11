#pragma once

#include "context.hpp"
#include "skeleton.hpp"
#include "view.hpp"

namespace ash::sample::mmd
{
class animation : public ash::core::system_base
{
public:
    static constexpr uuid id = "cb3c4adc-4849-4871-8857-9ee68a9999e2";

public:
    animation() : system_base("animation") {}

    virtual bool initialize(const ash::dictionary& config) override;

    void update();

private:
    ash::ecs::view<skeleton>* m_view;
};
} // namespace ash::sample::mmd