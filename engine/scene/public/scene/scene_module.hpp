#pragma once

#include "core/engine_module.hpp"
#include "math/math.hpp"

namespace violet
{
class scene_module : public engine_module
{
public:
    scene_module();

    virtual bool initialize(const dictionary& config) override;

    void update_bounding_box();

    void frustum_culling(const std::array<float4, 6>& frustum);

private:
};
} // namespace violet