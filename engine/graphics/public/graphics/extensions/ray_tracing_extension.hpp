#pragma once

#include "graphics/render_device.hpp"

namespace violet
{
class ray_tracing_extension : public render_device_extension
{
public:
    ray_tracing_extension();

private:
    std::unique_ptr<rhi_extension> create_rhi(rhi_backend backend) override;
};
} // namespace violet