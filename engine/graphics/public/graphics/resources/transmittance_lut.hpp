#pragma once

#include "graphics/atmosphere.hpp"
#include "graphics/resources/texture.hpp"

namespace violet
{
class transmittance_lut : public texture_2d
{
public:
    transmittance_lut(const atmosphere& atmosphere = {});
};
} // namespace violet