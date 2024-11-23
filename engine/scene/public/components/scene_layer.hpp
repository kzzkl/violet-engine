#pragma once

#include <cstdint>

namespace violet
{
class scene;
struct scene_layer
{
    scene* scene;
    std::uint32_t layer;
};
} // namespace violet