#pragma once

#include "core/engine.hpp"

namespace violet
{
class animation_system : public system
{
public:
    animation_system();

    bool initialize(const dictionary& config) override;

private:
    void update_skeleton();
};
} // namespace violet