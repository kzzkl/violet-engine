#pragma once

#include "core/engine.hpp"

namespace violet
{
class scene_system : public system
{
public:
    scene_system();

    void install(application& app) override;
    bool initialize(const dictionary& config) override;
};
} // namespace violet