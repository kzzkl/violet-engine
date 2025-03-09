#include "tools/texture_loader.hpp"
#include <array>
#include <cstddef>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace violet
{
namespace
{
std::vector<std::uint8_t> read_file(std::string_view path)
{
    std::ifstream fin(path.data(), std::ios_base::binary);
    if (!fin.is_open())
    {
        return {};
    }

    return {std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>()};
}
} // namespace

rhi_ptr<rhi_texture> texture_loader::load(std::string_view path, texture_options options)
{
    return load(std::span(&path, 1), options, false);
}

rhi_ptr<rhi_texture> texture_loader::load(
    std::string_view right,
    std::string_view left,
    std::string_view top,
    std::string_view bottom,
    std::string_view front,
    std::string_view back,
    texture_options options)
{
    std::array<std::string_view, 6> paths = {right, left, top, bottom, front, back};
    return load(paths, options, true);
}

rhi_ptr<rhi_texture> texture_loader::load(const texture_data& data, texture_options options)
{
    auto level_count = static_cast<std::uint32_t>(data.mipmaps.size());
    rhi_texture_extent extent = data.mipmaps[0].extent;

    bool need_generate_mipmaps =
        data.mipmaps.size() == 1 && (options & TEXTURE_OPTION_GENERATE_MIPMAPS);
    if (need_generate_mipmaps)
    {
        std::uint32_t max_size = std::max(extent.width, extent.height);
        level_count = static_cast<std::uint32_t>(std::floor(std::log2(max_size))) + 1;
    }

    rhi_texture_desc texture_desc = {
        .extent = extent,
        .format = data.format,
        .flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_TRANSFER_DST,
        .level_count = level_count,
        .layer_count = 1,
        .samples = RHI_SAMPLE_COUNT_1,
    };
    texture_desc.flags |= need_generate_mipmaps ? RHI_TEXTURE_TRANSFER_SRC : 0;
    rhi_ptr<rhi_texture> texture = render_device::instance().create_texture(texture_desc);

    rhi_command* command = render_device::instance().allocate_command();

    upload(command, data, texture.get());

    if (need_generate_mipmaps)
    {
        generate_mipmaps(command, texture.get(), level_count, 0);
    }

    rhi_texture_barrier texture_barrier = {
        .texture = texture.get(),
        .src_stages = RHI_PIPELINE_STAGE_TRANSFER,
        .src_access = need_generate_mipmaps ? RHI_ACCESS_TRANSFER_READ : RHI_ACCESS_TRANSFER_WRITE,
        .src_layout = need_generate_mipmaps ? RHI_TEXTURE_LAYOUT_TRANSFER_SRC :
                                              RHI_TEXTURE_LAYOUT_TRANSFER_DST,
        .dst_stages =
            RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_FRAGMENT | RHI_PIPELINE_STAGE_COMPUTE,
        .dst_access = RHI_ACCESS_SHADER_READ,
        .dst_layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
        .level = 0,
        .level_count = level_count,
        .layer = 0,
        .layer_count = 1,
    };

    command->set_pipeline_barrier(nullptr, 0, &texture_barrier, 1);

    render_device::instance().execute(command, true);

    return texture;
}

rhi_ptr<rhi_texture> texture_loader::load(
    std::span<std::string_view> paths,
    texture_options options,
    bool is_cube)
{
    assert(!is_cube || paths.size() == 6);

    std::optional<texture_data> data = load_file(paths[0], options);
    if (!data.has_value() || data->mipmaps.empty())
    {
        return nullptr;
    }

    auto layer_count = static_cast<std::uint32_t>(paths.size());
    auto level_count = static_cast<std::uint32_t>(data->mipmaps.size());
    rhi_texture_extent extent = data->mipmaps[0].extent;

    bool need_generate_mipmaps =
        data->mipmaps.size() == 1 && (options & TEXTURE_OPTION_GENERATE_MIPMAPS);
    if (need_generate_mipmaps)
    {
        std::uint32_t max_size = std::max(extent.width, extent.height);
        level_count = static_cast<std::uint32_t>(std::floor(std::log2(max_size))) + 1;
    }

    rhi_texture_desc texture_desc = {
        .extent = extent,
        .format = data->format,
        .flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_TRANSFER_DST,
        .level_count = level_count,
        .layer_count = 1,
        .samples = RHI_SAMPLE_COUNT_1,
    };
    texture_desc.flags |= need_generate_mipmaps ? RHI_TEXTURE_TRANSFER_SRC : 0;
    rhi_ptr<rhi_texture> texture = render_device::instance().create_texture(texture_desc);

    rhi_command* command = render_device::instance().allocate_command();

    for (std::uint32_t layer = 0; layer < layer_count; ++layer)
    {
        if (layer != 0)
        {
            data = load_file(paths[layer], options);
            if (!data.has_value() || data->mipmaps.empty())
            {
                return nullptr;
            }
        }

        assert(
            data->mipmaps[0].extent.width == extent.width &&
            data->mipmaps[0].extent.height == extent.height);

        upload(command, data.value(), texture.get());

        if (need_generate_mipmaps)
        {
            generate_mipmaps(command, texture.get(), level_count, layer);
        }
    }

    rhi_texture_barrier texture_barrier = {
        .texture = texture.get(),
        .src_stages = RHI_PIPELINE_STAGE_TRANSFER,
        .src_access = need_generate_mipmaps ? RHI_ACCESS_TRANSFER_READ : RHI_ACCESS_TRANSFER_WRITE,
        .src_layout = need_generate_mipmaps ? RHI_TEXTURE_LAYOUT_TRANSFER_SRC :
                                              RHI_TEXTURE_LAYOUT_TRANSFER_DST,
        .dst_stages =
            RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_FRAGMENT | RHI_PIPELINE_STAGE_COMPUTE,
        .dst_access = RHI_ACCESS_SHADER_READ,
        .dst_layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
        .level = 0,
        .level_count = level_count,
        .layer = 0,
        .layer_count = layer_count,
    };

    command->set_pipeline_barrier(nullptr, 0, &texture_barrier, 1);

    render_device::instance().execute(command, true);

    return texture;
}

std::optional<texture_data> texture_loader::load_file(
    std::string_view path,
    texture_options options)
{
    if (path.ends_with(".dds"))
    {
        return load_dds(path, options);
    }

    if (path.ends_with(".hdr"))
    {
        return load_hdr(path, options);
    }

    return load_other(path, options);
}

std::optional<texture_data> texture_loader::load_dds(std::string_view path, texture_options options)
{
#define DDPF_ALPHAPIXELS 0x1
#define DDPF_ALPHA 0x2
#define DDPF_FOURCC 0x4
#define DDPF_RGB 0x40
#define DDPF_YUV 0x200
#define DDPF_LUMINANCE 0x20000

#define FOURCC_DXT1 0x31545844
#define FOURCC_DXT3 0x33545844
#define FOURCC_DXT5 0x35545844

    struct dds_pixel_format
    {
        std::uint32_t dwSize;
        std::uint32_t dwFlags;
        std::uint32_t dwFourCC;
        std::uint32_t dwRGBBitCount;
        std::uint32_t dwRBitMask;
        std::uint32_t dwGBitMask;
        std::uint32_t dwBBitMask;
        std::uint32_t dwABitMask;
    };

    struct dds_header
    {
        std::uint32_t dwSize;
        std::uint32_t dwFlags;
        std::uint32_t dwHeight;
        std::uint32_t dwWidth;
        std::uint32_t dwPitchOrLinearSize;
        std::uint32_t dwDepth;
        std::uint32_t dwMipMapCount;
        std::uint32_t dwReserved1[11];
        dds_pixel_format ddspf;
        std::uint32_t dwCaps;
        std::uint32_t dwCaps2;
        std::uint32_t dwCaps3;
        std::uint32_t dwCaps4;
        std::uint32_t dwReserved2;
    };

    enum class dds_format
    {
        RGBA_EXT1,
        RGBA_EXT3,
        RGBA_EXT5,
        BGRA,
        RGBA,
        RGB,
        BGR,
        LUMINANCE
    };

    std::ifstream fin(path.data(), std::ios::binary);
    if (!fin.is_open())
    {
        return std::nullopt;
    }

    char magic[4] = {};
    fin.read(magic, 4);
    if (std::strncmp(magic, "DDS ", 4) != 0)
    {
        return std::nullopt;
    }

    dds_header header = {};
    fin.read(reinterpret_cast<char*>(&header), sizeof(dds_header));

    auto result = std::make_optional<texture_data>();

    dds_format format;
    std::uint32_t channels;
    if (header.ddspf.dwFlags & DDPF_FOURCC)
    {
        switch (header.ddspf.dwFourCC)
        {
        case FOURCC_DXT1:
            result->format = RHI_FORMAT_R8G8B8A8_UNORM;
            format = dds_format::RGBA_EXT1;
            channels = 3;
            break;
        case FOURCC_DXT3:
            result->format = RHI_FORMAT_R8G8B8A8_UNORM;
            format = dds_format::RGBA_EXT3;
            channels = 4;
            break;
        case FOURCC_DXT5:
            result->format = RHI_FORMAT_R8G8B8A8_UNORM;
            format = dds_format::RGBA_EXT5;
            channels = 4;
            break;
        default:
            return std::nullopt;
        }
    }
    else if (
        header.ddspf.dwRGBBitCount == 32 && header.ddspf.dwRBitMask == 0x00FF0000 &&
        header.ddspf.dwGBitMask == 0x0000FF00 && header.ddspf.dwBBitMask == 0x000000FF &&
        header.ddspf.dwABitMask == 0xFF000000)
    {
        result->format = RHI_FORMAT_B8G8R8A8_UNORM;
        format = dds_format::BGRA;
        channels = 4;
    }
    else if (
        header.ddspf.dwRGBBitCount == 32 && header.ddspf.dwRBitMask == 0x000000FF &&
        header.ddspf.dwGBitMask == 0x0000FF00 && header.ddspf.dwBBitMask == 0x00FF0000 &&
        header.ddspf.dwABitMask == 0xFF000000)
    {
        result->format = RHI_FORMAT_R8G8B8A8_UNORM;
        format = dds_format::RGBA;
        channels = 4;
    }
    else if (
        header.ddspf.dwRGBBitCount == 24 && header.ddspf.dwRBitMask == 0x000000FF &&
        header.ddspf.dwGBitMask == 0x0000FF00 && header.ddspf.dwBBitMask == 0x00FF0000)
    {
        result->format = RHI_FORMAT_R8G8B8_UNORM;
        format = dds_format::RGB;
        channels = 3;
    }
    else if (
        header.ddspf.dwRGBBitCount == 24 && header.ddspf.dwRBitMask == 0x00FF0000 &&
        header.ddspf.dwGBitMask == 0x0000FF00 && header.ddspf.dwBBitMask == 0x000000FF)
    {
        result->format = RHI_FORMAT_B8G8R8_UNORM;
        format = dds_format::BGR;
        channels = 3;
    }
    else if (header.ddspf.dwRGBBitCount == 8)
    {
        format = dds_format::LUMINANCE;
        channels = 1;
    }
    else
    {
        return std::nullopt;
    }

    auto calculate_texture_size =
        [](std::uint32_t width, std::uint32_t height, std::uint32_t channels, dds_format format)
    {
        if (format == dds_format::RGBA_EXT1 || format == dds_format::RGBA_EXT3 ||
            format == dds_format::RGBA_EXT5)
        {
            return ((width + 3) / 4) * ((height + 3) / 4) *
                   (format == dds_format::RGBA_EXT1 ? 8 : 16);
        }

        return width * height * channels;
    };

    std::uint32_t mip_width = header.dwWidth;
    std::uint32_t mip_height = header.dwHeight;
    for (std::uint32_t i = 0; i < header.dwMipMapCount; ++i)
    {
        texture_data::mipmap mipmap;
        mipmap.extent.width = mip_width;
        mipmap.extent.height = mip_height;

        std::size_t size = calculate_texture_size(mip_width, mip_height, channels, format);
        mipmap.pixels.resize(size);
        fin.read(mipmap.pixels.data(), static_cast<long long>(size));

        mip_width = mip_width >> 1;
        mip_height = mip_height >> 1;

        result->mipmaps.push_back(std::move(mipmap));
    }

    fin.close();

    return result;
}

std::optional<texture_data> texture_loader::load_hdr(std::string_view path, texture_options options)
{
    int width;
    int height;
    int channels;

    float* pixels = stbi_loadf(path.data(), &width, &height, &channels, STBI_default);
    if (pixels == nullptr)
    {
        return std::nullopt;
    }

    std::size_t pixel_count = static_cast<std::size_t>(width) * height;

    texture_data::mipmap mipmap;
    mipmap.extent.width = static_cast<std::uint32_t>(width);
    mipmap.extent.height = static_cast<std::uint32_t>(height);
    mipmap.pixels.resize(pixel_count * sizeof(float) * 4);

    auto* target = reinterpret_cast<float*>(mipmap.pixels.data());

    for (std::size_t i = 0; i < pixel_count; ++i)
    {
        target[i * 4 + 0] = pixels[i * 3 + 0];
        target[i * 4 + 1] = pixels[i * 3 + 1];
        target[i * 4 + 2] = pixels[i * 3 + 2];
        target[i * 4 + 3] = 1.0f;
    }

    stbi_image_free(pixels);

    auto result = std::make_optional<texture_data>();
    result->mipmaps.push_back(std::move(mipmap));
    result->format = RHI_FORMAT_R32G32B32A32_FLOAT;

    return result;
}

std::optional<texture_data> texture_loader::load_other(
    std::string_view path,
    texture_options options)
{
    int width;
    int height;
    int channels;

    std::vector<std::uint8_t> data = read_file(path);
    stbi_uc* pixels = stbi_load_from_memory(
        data.data(),
        static_cast<int>(data.size()),
        &width,
        &height,
        &channels,
        STBI_rgb_alpha);
    if (pixels == nullptr)
    {
        return std::nullopt;
    }

    std::size_t image_size = static_cast<std::size_t>(width * height) * 4;

    texture_data::mipmap mipmap;
    mipmap.extent.width = static_cast<std::uint32_t>(width);
    mipmap.extent.height = static_cast<std::uint32_t>(height);
    mipmap.pixels.resize(image_size);

    std::memcpy(mipmap.pixels.data(), pixels, image_size);

    stbi_image_free(pixels);

    auto result = std::make_optional<texture_data>();
    result->mipmaps.push_back(std::move(mipmap));
    result->format =
        (options & TEXTURE_OPTION_SRGB) ? RHI_FORMAT_R8G8B8A8_SRGB : RHI_FORMAT_R8G8B8A8_UNORM;

    return result;
}

void texture_loader::upload(
    rhi_command* command,
    const texture_data& data,
    rhi_texture* texture,
    std::uint32_t layer)
{
    rhi_buffer_desc staging_buffer_desc = {
        .flags = RHI_BUFFER_TRANSFER_SRC | RHI_BUFFER_HOST_VISIBLE,
    };

    for (const auto& mipmap : data.mipmaps)
    {
        staging_buffer_desc.size += mipmap.pixels.size();
    }

    rhi_ptr<rhi_buffer> staging_buffer =
        render_device::instance().create_buffer(staging_buffer_desc);

    auto* buffer = static_cast<std::uint8_t*>(staging_buffer->get_buffer_pointer());
    for (const auto& mipmap : data.mipmaps)
    {
        std::memcpy(buffer, mipmap.pixels.data(), mipmap.pixels.size());
        buffer += mipmap.pixels.size();
    }

    rhi_buffer_barrier buffer_barrier = {
        .buffer = staging_buffer.get(),
        .src_stages = RHI_PIPELINE_STAGE_HOST,
        .src_access = RHI_ACCESS_HOST_WRITE,
        .dst_stages = RHI_PIPELINE_STAGE_TRANSFER,
        .dst_access = RHI_ACCESS_TRANSFER_READ,
    };

    rhi_texture_barrier texture_barrier = {
        .texture = texture,
        .src_stages = RHI_PIPELINE_STAGE_HOST | RHI_PIPELINE_STAGE_TRANSFER,
        .src_access = 0,
        .src_layout = RHI_TEXTURE_LAYOUT_UNDEFINED,
        .dst_stages = RHI_PIPELINE_STAGE_TRANSFER,
        .dst_access = RHI_ACCESS_TRANSFER_WRITE,
        .dst_layout = RHI_TEXTURE_LAYOUT_TRANSFER_DST,
        .level_count = 1,
        .layer = layer,
        .layer_count = 1,
    };

    for (std::uint32_t level = 0; level < data.mipmaps.size(); ++level)
    {
        const auto& mipmap = data.mipmaps[level];

        buffer_barrier.size = mipmap.pixels.size();
        texture_barrier.level = level;

        command->set_pipeline_barrier(&buffer_barrier, 1, &texture_barrier, 1);

        rhi_buffer_region buffer_region = {
            .offset = buffer_barrier.offset,
            .size = mipmap.pixels.size(),
        };

        rhi_texture_region texture_region = {
            .extent = mipmap.extent,
            .level = 0,
            .layer = 0,
            .layer_count = 1,
        };

        command
            ->copy_buffer_to_texture(staging_buffer.get(), buffer_region, texture, texture_region);

        buffer_barrier.offset += mipmap.pixels.size();
    }
}

void texture_loader::generate_mipmaps(
    rhi_command* command,
    rhi_texture* texture,
    std::uint32_t level_count,
    std::uint32_t layer)
{
    rhi_texture_extent extent = texture->get_extent();

    std::array<rhi_texture_barrier, 2> texture_barriers;
    texture_barriers[0] = {
        .texture = texture,
        .src_stages = RHI_PIPELINE_STAGE_TRANSFER,
        .src_access = RHI_ACCESS_TRANSFER_WRITE,
        .src_layout = RHI_TEXTURE_LAYOUT_TRANSFER_DST,
        .dst_stages = RHI_PIPELINE_STAGE_TRANSFER,
        .dst_access = RHI_ACCESS_TRANSFER_READ,
        .dst_layout = RHI_TEXTURE_LAYOUT_TRANSFER_SRC,
        .level = 0,
        .level_count = 1,
        .layer = layer,
        .layer_count = 1,
    };

    texture_barriers[1] = {
        .texture = texture,
        .src_stages = RHI_PIPELINE_STAGE_TRANSFER,
        .src_access = RHI_ACCESS_TRANSFER_WRITE,
        .src_layout = RHI_TEXTURE_LAYOUT_UNDEFINED,
        .dst_stages = RHI_PIPELINE_STAGE_TRANSFER,
        .dst_access = RHI_ACCESS_TRANSFER_WRITE,
        .dst_layout = RHI_TEXTURE_LAYOUT_TRANSFER_DST,
        .level = 0,
        .level_count = 1,
        .layer = layer,
        .layer_count = 1,
    };

    for (std::size_t i = 1; i < level_count; ++i)
    {
        texture_barriers[0].level = i - 1;
        texture_barriers[1].level = i;

        command->set_pipeline_barrier(nullptr, 0, texture_barriers.data(), 2);

        rhi_texture_region src_region = {
            .offset_x = 0,
            .offset_y = 0,
            .extent = extent,
            .level = static_cast<std::uint32_t>(i - 1),
            .layer = layer,
            .layer_count = 1,
        };

        extent.width = std::max(extent.width / 2, 1u);
        extent.height = std::max(extent.height / 2, 1u);

        rhi_texture_region dst_region = {
            .offset_x = 0,
            .offset_y = 0,
            .extent = extent,
            .level = static_cast<std::uint32_t>(i),
            .layer = layer,
            .layer_count = 1,
        };

        command->blit_texture(texture, src_region, texture, dst_region);
    }

    texture_barriers[0].level = level_count - 1;
    command->set_pipeline_barrier(nullptr, 0, texture_barriers.data(), 1);
}
} // namespace violet