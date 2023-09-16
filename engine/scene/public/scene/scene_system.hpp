#pragma once

#include "core/engine_system.hpp"
#include "math/math.hpp"

namespace violet
{
class scene_system : public engine_system
{
public:
    scene_system();

    virtual bool initialize(const dictionary& config) override;

    void update_bounding_box();

    void frustum_culling(const std::array<float4, 6>& frustum);

private:
};
} // namespace violet