#pragma once

#include "core/context/engine_module.hpp"
#include "math/math.hpp"

namespace violet::scene
{
class scene : public engine_module
{
public:
    scene();

    virtual bool initialize(const dictionary& config) override;
    virtual void on_end_frame() override;

    void update_transform();
    void update_bounding_box();

    void frustum_culling(const std::array<float4, 6>& frustum);
};
} // namespace violet::scene