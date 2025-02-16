#pragma once

#include "graphics/resources/texture.hpp"

namespace violet
{
class hilbert_lut : public texture_2d
{
public:
    hilbert_lut(std::uint32_t level = 6);

private:
    static std::uint32_t hilbert_index(std::uint32_t x, std::uint32_t y, std::uint32_t level);
};
} // namespace violet