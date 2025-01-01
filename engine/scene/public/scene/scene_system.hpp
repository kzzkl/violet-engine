#pragma once

#include "core/engine_system.hpp"

namespace violet
{
class scene_system : public engine_system
{
public:
    scene_system();

    bool initialize(const dictionary& config) override;
};
} // namespace violet