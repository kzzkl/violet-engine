#pragma once

#include "graphics/render_device.hpp"
#include "graphics/resources/texture.hpp"

namespace violet
{
class texture_loader
{
public:
    enum load_option
    {
        LOAD_OPTION_NONE = 0,
        LOAD_OPTION_CUBE_MAP = 1 << 0,
    };
    using load_options = std::uint32_t;

    static rhi_ptr<rhi_texture> load(
        const texture_data& data,
        load_options options = LOAD_OPTION_NONE);

private:
    static void upload(rhi_command* command, const texture_data& data, rhi_texture* texture);
};
} // namespace violet