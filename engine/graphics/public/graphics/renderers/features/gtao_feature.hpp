#pragma once

#include "graphics/renderer.hpp"

namespace violet
{
class gtao_feature : public render_feature<gtao_feature>
{
public:
    std::uint32_t slice_count{3};
    std::uint32_t step_count{3};
    float radius{1.5f};
    float falloff{0.8f};
};
} // namespace violet