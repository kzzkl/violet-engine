#pragma once

#include "graphics/render_device.hpp"

namespace violet
{
class core_extension : public render_device_extension
{
public:
    core_extension();

private:
    std::unique_ptr<rhi_extension> create_rhi(rhi_backend backend) override;
};
} // namespace violet