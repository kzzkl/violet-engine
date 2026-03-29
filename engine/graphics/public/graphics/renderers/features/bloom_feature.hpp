#pragma once

#include "graphics/renderer.hpp"

namespace violet
{
class bloom_feature : public render_feature<bloom_feature>
{
public:
    float threshold{0.9f};
    float intensity{0.1f};
    float knee{0.25f};
    float radius{0.5f};
};
} // namespace violet