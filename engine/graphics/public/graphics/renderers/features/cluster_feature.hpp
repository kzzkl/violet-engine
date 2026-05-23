#pragma once

#include "graphics/renderer.hpp"

namespace violet
{
struct cluster_feature : public render_feature<cluster_feature>
{
    float threshold{1.0f};
};
} // namespace violet