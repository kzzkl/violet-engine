#pragma once

#include "graphics/renderer.hpp"

namespace violet
{
class atmosphere_feature : public render_feature<atmosphere_feature>
{
public:
    bool enable_multi_scattering{true};
    bool enable_shadow{false};
    std::uint32_t ibl_update_interval{10};
};
} // namespace violet