#pragma once

#include "graphics/resources/texture.hpp"

namespace violet
{
class brdf_lut : public texture_2d
{
public:
    brdf_lut(std::uint32_t size = 128);
};
} // namespace violet