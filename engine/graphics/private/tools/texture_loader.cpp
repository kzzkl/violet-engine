#include "graphics/tools/texture_loader.hpp"
#include <array>
#include <cstddef>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace violet
{
rhi_ptr<rhi_texture> texture_loader::load(std::string_view path, texture_load_option options)
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
    texture_load_option options)
{
    std::array<std::string_view, 6> paths = {right, left, top, bottom, front, back};
    return load(paths, options, true);
}

rhi_ptr<rhi_texture> texture_loader::load(
    std::span<std::string_view> paths,
    texture_load_option options,
    bool is_cube)
{
    assert(!is_cube || paths.size() == 6);

    auto load_file = [options](std::string_view path) -> std::optional<texture_data>
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
    };

    std::optional<texture_data> data = load_file(paths[0]);
    if (!data.has_value() || data->mipmaps.empty())
    {
        return nullptr;
    }

    auto layer_count = static_cast<std::uint32_t>(paths.size());
    auto level_count = static_cast<std::uint32_t>(data->mipmaps.size());
    rhi_texture_extent extent = data->mipmaps[0].extent;

    bool need_generate_mipmaps =
        data->mipmaps.size() == 1 && (options & TEXTURE_LOAD_OPTION_GENERATE_MIPMAPS);
    if (need_generate_mipmaps)
    {
        std::uint32_t max_size = std::max(extent.width, extent.height);
        level_count = static_cast<std::uint32_t>(std::floor(std::log2(max_size))) + 1;
    }

    rhi_texture_desc texture_desc = {};
    texture_desc.extent = extent;
    texture_desc.format = data->format;
    texture_desc.level_count = level_count;
    texture_desc.layer_count = 1;
    texture_desc.samples = RHI_SAMPLE_COUNT_1;
    texture_desc.flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_TRANSFER_DST;
    texture_desc.flags |= need_generate_mipmaps ? RHI_TEXTURE_TRANSFER_SRC : 0;
    rhi_ptr<rhi_texture> texture = render_device::instance().create_texture(texture_desc);

    rhi_command* command = render_device::instance().allocate_command();

    for (std::uint32_t layer = 0; layer < layer_count; ++layer)
    {
        if (layer != 0)
        {
            data = load_file(paths[layer]);
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

            rhi_texture_barrier texture_barrier = {};
            texture_barrier.texture = texture.get();
            texture_barrier.src_access = RHI_ACCESS_TRANSFER_READ;
            texture_barrier.dst_access = RHI_ACCESS_SHADER_READ;
            texture_barrier.src_layout = RHI_TEXTURE_LAYOUT_TRANSFER_SRC;
            texture_barrier.dst_layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE;
            texture_barrier.level = 0;
            texture_barrier.level_count = level_count;
            texture_barrier.layer = layer;
            texture_barrier.layer_count = 1;
            command->set_pipeline_barrier(
                RHI_PIPELINE_STAGE_TRANSFER,
                RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_FRAGMENT |
                    RHI_PIPELINE_STAGE_COMPUTE,
                nullptr,
                0,
                &texture_barrier,
                1);
        }
        else
        {
            rhi_texture_barrier texture_barrier = {};
            texture_barrier.texture = texture.get();
            texture_barrier.src_access = RHI_ACCESS_TRANSFER_WRITE;
            texture_barrier.dst_access = RHI_ACCESS_SHADER_READ;
            texture_barrier.src_layout = RHI_TEXTURE_LAYOUT_TRANSFER_DST;
            texture_barrier.dst_layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE;
            texture_barrier.level = 0;
            texture_barrier.level_count = 1;
            texture_barrier.layer = layer;
            texture_barrier.layer_count = 1;
            command->set_pipeline_barrier(
                RHI_PIPELINE_STAGE_TRANSFER,
                RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_FRAGMENT |
                    RHI_PIPELINE_STAGE_COMPUTE,
                nullptr,
                0,
                &texture_barrier,
                1);
        }
    }

    render_device::instance().execute(command);

    return texture;
}

rhi_ptr<rhi_texture> texture_loader::load(const texture_data& data, texture_load_option options)
{
    std::uint32_t level_count = static_cast<std::uint32_t>(data.mipmaps.size());
    rhi_texture_extent extent = data.mipmaps[0].extent;

    bool need_generate_mipmaps =
        data.mipmaps.size() == 1 && (options & TEXTURE_LOAD_OPTION_GENERATE_MIPMAPS);
    if (need_generate_mipmaps)
    {
        std::uint32_t max_size = std::max(extent.width, extent.height);
        level_count = static_cast<std::uint32_t>(std::floor(std::log2(max_size))) + 1;
    }

    rhi_texture_desc texture_desc = {};
    texture_desc.extent = extent;
    texture_desc.format = data.format;
    texture_desc.level_count = level_count;
    texture_desc.layer_count = 1;
    texture_desc.samples = RHI_SAMPLE_COUNT_1;
    texture_desc.flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_TRANSFER_DST;
    texture_desc.flags |= need_generate_mipmaps ? RHI_TEXTURE_TRANSFER_SRC : 0;
    rhi_ptr<rhi_texture> texture = render_device::instance().create_texture(texture_desc);

    rhi_command* command = render_device::instance().allocate_command();

    upload(command, data, texture.get());

    if (need_generate_mipmaps)
    {
        generate_mipmaps(command, texture.get(), level_count, 0);

        rhi_texture_barrier texture_barrier = {};
        texture_barrier.texture = texture.get();
        texture_barrier.src_access = RHI_ACCESS_TRANSFER_READ;
        texture_barrier.dst_access = RHI_ACCESS_SHADER_READ;
        texture_barrier.src_layout = RHI_TEXTURE_LAYOUT_TRANSFER_SRC;
        texture_barrier.dst_layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE;
        texture_barrier.level = 0;
        texture_barrier.level_count = level_count;
        texture_barrier.layer = 0;
        texture_barrier.layer_count = 1;
        command->set_pipeline_barrier(
            RHI_PIPELINE_STAGE_TRANSFER,
            RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_FRAGMENT | RHI_PIPELINE_STAGE_COMPUTE,
            nullptr,
            0,
            &texture_barrier,
            1);
    }
    else
    {
        rhi_texture_barrier texture_barrier = {};
        texture_barrier.texture = texture.get();
        texture_barrier.src_access = RHI_ACCESS_TRANSFER_WRITE;
        texture_barrier.dst_access = RHI_ACCESS_SHADER_READ;
        texture_barrier.src_layout = RHI_TEXTURE_LAYOUT_TRANSFER_DST;
        texture_barrier.dst_layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE;
        texture_barrier.level = 0;
        texture_barrier.level_count = 1;
        texture_barrier.layer = 0;
        texture_barrier.layer_count = 1;
        command->set_pipeline_barrier(
            RHI_PIPELINE_STAGE_TRANSFER,
            RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_FRAGMENT | RHI_PIPELINE_STAGE_COMPUTE,
            nullptr,
            0,
            &texture_barrier,
            1);
    }

    render_device::instance().execute(command);

    return texture;
}

std::optional<texture_loader::texture_data> texture_loader::load_dds(
    std::string_view path,
    texture_load_option options)
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
        mipmap_data mipmap;
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

std::optional<texture_loader::texture_data> texture_loader::load_hdr(
    std::string_view path,
    texture_load_option options)
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

    mipmap_data mipmap;
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

std::optional<texture_loader::texture_data> texture_loader::load_other(
    std::string_view path,
    texture_load_option options)
{
    int width;
    int height;
    int channels;

    stbi_uc* pixels = stbi_load(path.data(), &width, &height, &channels, STBI_rgb_alpha);
    if (pixels == nullptr)
    {
        return std::nullopt;
    }

    std::size_t image_size = static_cast<std::size_t>(width * height) * 4;

    mipmap_data mipmap;
    mipmap.extent.width = static_cast<std::uint32_t>(width);
    mipmap.extent.height = static_cast<std::uint32_t>(height);
    mipmap.pixels.resize(image_size);

    std::memcpy(mipmap.pixels.data(), pixels, image_size);

    stbi_image_free(pixels);

    auto result = std::make_optional<texture_data>();
    result->mipmaps.push_back(std::move(mipmap));
    result->format =
        (options & TEXTURE_LOAD_OPTION_SRGB) ? RHI_FORMAT_R8G8B8A8_SRGB : RHI_FORMAT_R8G8B8A8_UNORM;

    return result;
}

void texture_loader::upload(
    rhi_command* command,
    const texture_data& data,
    rhi_texture* texture,
    std::uint32_t layer)
{
    rhi_buffer_desc staging_buffer_desc = {};
    staging_buffer_desc.flags = RHI_BUFFER_TRANSFER_SRC | RHI_BUFFER_HOST_VISIBLE;
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

    rhi_buffer_barrier buffer_barrier = {};
    buffer_barrier.buffer = staging_buffer.get();
    buffer_barrier.src_access = RHI_ACCESS_HOST_WRITE;
    buffer_barrier.dst_access = RHI_ACCESS_TRANSFER_READ;

    rhi_texture_barrier texture_barrier = {};
    texture_barrier.texture = texture;
    texture_barrier.src_access = 0;
    texture_barrier.dst_access = RHI_ACCESS_TRANSFER_WRITE;
    texture_barrier.src_layout = RHI_TEXTURE_LAYOUT_UNDEFINED;
    texture_barrier.dst_layout = RHI_TEXTURE_LAYOUT_TRANSFER_DST;
    texture_barrier.level_count = 1;
    texture_barrier.layer = layer;
    texture_barrier.layer_count = 1;

    for (std::uint32_t level = 0; level < data.mipmaps.size(); ++level)
    {
        const auto& mipmap = data.mipmaps[level];

        buffer_barrier.size = mipmap.pixels.size();
        texture_barrier.level = level;

        command->set_pipeline_barrier(
            RHI_PIPELINE_STAGE_HOST | RHI_PIPELINE_STAGE_TRANSFER,
            RHI_PIPELINE_STAGE_TRANSFER,
            &buffer_barrier,
            1,
            &texture_barrier,
            1);

        rhi_buffer_region buffer_region = {};
        buffer_region.offset = buffer_barrier.offset;
        buffer_region.size = mipmap.pixels.size();

        rhi_texture_region texture_region = {};
        texture_region.level = 0;
        texture_region.layer = 0;
        texture_region.layer_count = 1;
        texture_region.extent = mipmap.extent;

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
    texture_barriers[0].texture = texture;
    texture_barriers[0].src_access = RHI_ACCESS_TRANSFER_WRITE;
    texture_barriers[0].dst_access = RHI_ACCESS_TRANSFER_READ;
    texture_barriers[0].src_layout = RHI_TEXTURE_LAYOUT_TRANSFER_DST;
    texture_barriers[0].dst_layout = RHI_TEXTURE_LAYOUT_TRANSFER_SRC;
    texture_barriers[0].level = 0;
    texture_barriers[0].level_count = 1;
    texture_barriers[0].layer = layer;
    texture_barriers[0].layer_count = 1;

    texture_barriers[1].texture = texture;
    texture_barriers[1].src_access = RHI_ACCESS_TRANSFER_WRITE;
    texture_barriers[1].dst_access = RHI_ACCESS_TRANSFER_WRITE;
    texture_barriers[1].src_layout = RHI_TEXTURE_LAYOUT_UNDEFINED;
    texture_barriers[1].dst_layout = RHI_TEXTURE_LAYOUT_TRANSFER_DST;
    texture_barriers[1].level = 0;
    texture_barriers[1].level_count = 1;
    texture_barriers[1].layer = layer;
    texture_barriers[1].layer_count = 1;

    for (std::size_t i = 1; i < level_count; ++i)
    {
        texture_barriers[0].level = i - 1;
        texture_barriers[1].level = i;

        command->set_pipeline_barrier(
            RHI_PIPELINE_STAGE_TRANSFER,
            RHI_PIPELINE_STAGE_TRANSFER,
            nullptr,
            0,
            texture_barriers.data(),
            2);

        rhi_texture_region src_region = {};
        src_region.level = i - 1;
        src_region.layer = layer;
        src_region.layer_count = 1;
        src_region.offset_x = 0;
        src_region.offset_y = 0;
        src_region.extent = extent;

        extent.width = std::max(extent.width / 2, 1u);
        extent.height = std::max(extent.height / 2, 1u);

        rhi_texture_region dst_region = {};
        dst_region.level = i;
        dst_region.layer = layer;
        dst_region.layer_count = 1;
        dst_region.offset_x = 0;
        dst_region.offset_y = 0;
        dst_region.extent = extent;

        command->blit_texture(texture, src_region, texture, dst_region);
    }

    texture_barriers[0].level = level_count - 1;
    command->set_pipeline_barrier(
        RHI_PIPELINE_STAGE_TRANSFER,
        RHI_PIPELINE_STAGE_TRANSFER,
        nullptr,
        0,
        texture_barriers.data(),
        1);
}
} // namespace violet