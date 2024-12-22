#pragma once

#include "core/engine_system.hpp"

namespace violet
{
class animation_system : public engine_system
{
public:
    animation_system();

    bool initialize(const dictionary& config) override;

private:
    void update_skeleton();
};
} // namespace violet