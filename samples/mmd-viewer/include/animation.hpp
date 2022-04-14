#pragma once

#include "context.hpp"
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
};
} // namespace ash::sample::mmd