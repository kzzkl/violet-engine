#include "animation/animation_system.hpp"

namespace violet
{
animation_system::animation_system()
    : system("animation")
{
}

bool animation_system::initialize(const dictionary& config)
{
    return true;
}
} // namespace violet