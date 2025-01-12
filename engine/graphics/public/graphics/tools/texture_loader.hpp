#pragma once

#include "graphics/render_device.hpp"
#include <optional>

namespace violet
{
enum texture_load_option
{
    TEXTURE_LOAD_OPTION_NONE = 0,
    TEXTURE_LOAD_OPTION_GENERATE_MIPMAPS = 1 << 0,
    TEXTURE_LOAD_OPTION_SRGB = 1 << 1
};
using texture_load_options = std::uint32_t;

class texture_loader
{
public:
    struct mipmap_data
    {
        rhi_texture_extent extent;
        std::vector<char> pixels;
    };

    struct texture_data
    {
        std::vector<mipmap_data> mipmaps;
        rhi_format format;
    };

    static rhi_ptr<rhi_texture> load(
        std::string_view path,
        texture_load_options options = TEXTURE_LOAD_OPTION_NONE);
    static rhi_ptr<rhi_texture> load(
        std::string_view right,
        std::string_view left,
        std::string_view top,
        std::string_view bottom,
        std::string_view front,
        std::string_view back,
        texture_load_options options = TEXTURE_LOAD_OPTION_NONE);
    static rhi_ptr<rhi_texture> load(
        const texture_data& data,
        texture_load_options options = TEXTURE_LOAD_OPTION_NONE);

private:
    static rhi_ptr<rhi_texture> load(
        std::span<std::string_view> paths,
        texture_load_options options,
        bool is_cube);

    static std::optional<texture_data> load_file(
        std::string_view path,
        texture_load_options options);
    static std::optional<texture_data> load_dds(
        std::string_view path,
        texture_load_options options);
    static std::optional<texture_data> load_hdr(
        std::string_view path,
        texture_load_options options);
    static std::optional<texture_data> load_other(
        std::string_view path,
        texture_load_options options);

    static void upload(
        rhi_command* command,
        const texture_data& data,
        rhi_texture* texture,
        std::uint32_t layer = 0);
    static void generate_mipmaps(
        rhi_command* command,
        rhi_texture* texture,
        std::uint32_t level_count,
        std::uint32_t layer);
};
} // namespace violet