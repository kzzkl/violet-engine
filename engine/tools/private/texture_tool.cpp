#include "tools/texture_tool.hpp"
#include "dds.hpp"
#include <compressonator.h>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>

namespace violet
{
namespace
{
std::vector<std::uint8_t> read_file(std::string_view path)
{
    std::ifstream fin(std::string(path), std::ios_base::binary);
    if (!fin.is_open())
    {
        return {};
    }

    return {std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>()};
}

CMP_FORMAT get_cmp_format(rhi_format format)
{
    switch (format)
    {
    case RHI_FORMAT_R8_UNORM:
        return CMP_FORMAT_R_8;
    case RHI_FORMAT_R8_SNORM:
        return CMP_FORMAT_R_8_S;
    case RHI_FORMAT_R8G8_UNORM:
        return CMP_FORMAT_RG_8;
    case RHI_FORMAT_R8G8_SNORM:
        return CMP_FORMAT_RG_8_S;
    case RHI_FORMAT_R8G8B8_UNORM:
        return CMP_FORMAT_RGB_888;
    case RHI_FORMAT_R8G8B8_SNORM:
        return CMP_FORMAT_RGB_888_S;
    case RHI_FORMAT_R8G8B8A8_UNORM:
    case RHI_FORMAT_R8G8B8A8_SRGB:
        return CMP_FORMAT_RGBA_8888;
    case RHI_FORMAT_R8G8B8A8_SNORM:
        return CMP_FORMAT_RGBA_8888_S;
    case RHI_FORMAT_B8G8R8_UNORM:
    case RHI_FORMAT_B8G8R8_SNORM:
        return CMP_FORMAT_BGR_888;
    case RHI_FORMAT_B8G8R8A8_UNORM:
    case RHI_FORMAT_B8G8R8A8_SRGB:
        return CMP_FORMAT_BGRA_8888;
    case RHI_FORMAT_R16G16_UNORM:
        return CMP_FORMAT_RG_16;
    case RHI_FORMAT_R16G16_FLOAT:
        return CMP_FORMAT_RG_16F;
    case RHI_FORMAT_R16G16B16A16_UNORM:
        return CMP_FORMAT_RGBA_16;
    case RHI_FORMAT_R16G16B16A16_FLOAT:
        return CMP_FORMAT_RGBA_16F;
    case RHI_FORMAT_R32_FLOAT:
        return CMP_FORMAT_R_32F;
    case RHI_FORMAT_R32G32_FLOAT:
        return CMP_FORMAT_RG_32F;
    case RHI_FORMAT_R32G32B32_FLOAT:
        return CMP_FORMAT_RGB_32F;
    case RHI_FORMAT_R32G32B32A32_FLOAT:
        return CMP_FORMAT_RGBA_32F;
    case RHI_FORMAT_BC1_RGB_UNORM:
    case RHI_FORMAT_BC1_RGB_SRGB:
    case RHI_FORMAT_BC1_RGBA_UNORM:
    case RHI_FORMAT_BC1_RGBA_SRGB:
        return CMP_FORMAT_BC1;
    case RHI_FORMAT_BC3_UNORM:
    case RHI_FORMAT_BC3_SRGB:
        return CMP_FORMAT_BC3;
    case RHI_FORMAT_BC5_UNORM:
        return CMP_FORMAT_BC5;
    case RHI_FORMAT_BC7_UNORM:
    case RHI_FORMAT_BC7_SRGB:
        return CMP_FORMAT_BC7;
    default:
        return CMP_FORMAT_Unknown;
    }
}

struct pixel_layout
{
    std::size_t total_size;

    std::vector<std::size_t> level_size;
    std::vector<std::size_t> offsets;

    std::size_t get_offset(std::uint32_t layer, std::uint32_t level) const
    {
        return offsets[(level_size.size() * layer) + level];
    }
};

pixel_layout get_pixels_layout(
    rhi_format format,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t layer_count,
    std::uint32_t level_count)
{
    const rhi_format_size format_size = rhi_get_format_size(format);

    pixel_layout layout = {};

    std::size_t layer_size = 0;

    for (std::uint32_t level = 0; level < level_count; ++level)
    {
        std::uint32_t level_width = std::max(width >> level, 1u);
        std::uint32_t level_height = std::max(height >> level, 1u);

        auto block_count_x = static_cast<std::uint32_t>(std::ceil(
            static_cast<float>(level_width) / static_cast<float>(format_size.block_width)));
        block_count_x = std::max(block_count_x, 1u);

        auto block_count_y = static_cast<std::uint32_t>(std::ceil(
            static_cast<float>(level_height) / static_cast<float>(format_size.block_height)));
        block_count_y = std::max(block_count_y, 1u);

        std::size_t level_size =
            static_cast<std::size_t>(block_count_x * block_count_y) * format_size.block_size;
        layout.level_size.push_back(level_size);

        layer_size += level_size;
    }

    std::size_t offset = 0;
    for (std::uint32_t layer = 0; layer < layer_count; ++layer)
    {
        for (std::uint32_t level = 0; level < level_count; ++level)
        {
            layout.offsets.push_back(offset);
            offset += layout.level_size[level];
        }
    }

    layout.total_size = layer_size * layer_count;

    return layout;
}
} // namespace

bool texture_tool::load(std::string_view path, texture_data& data)
{
    if (path.ends_with(".dds"))
    {
        return dds::load(path, data);
    }

    int width;
    int height;
    int channels;

    std::vector<std::uint8_t> file_data = read_file(path);
    stbi_uc* pixels = stbi_load_from_memory(
        file_data.data(),
        static_cast<int>(file_data.size()),
        &width,
        &height,
        &channels,
        STBI_rgb_alpha);

    if (pixels == nullptr)
    {
        return false;
    }

    std::size_t image_size = static_cast<std::size_t>(width * height) * 4;

    data = {
        .format = RHI_FORMAT_R8G8B8A8_UNORM,
        .extent =
            {
                .width = static_cast<std::uint32_t>(width),
                .height = static_cast<std::uint32_t>(height),
                .depth = 1,
            },
        .layer_count = 1,
        .level_count = 1,
    };
    data.pixels.resize(image_size);

    std::memcpy(data.pixels.data(), pixels, image_size);

    stbi_image_free(pixels);

    return true;
}

bool texture_tool::compress(const texture_data& src, texture_data& dst)
{
    assert(
        dst.format == RHI_FORMAT_BC1_RGB_UNORM || dst.format == RHI_FORMAT_BC1_RGB_SRGB ||
        dst.format == RHI_FORMAT_BC1_RGBA_UNORM || dst.format == RHI_FORMAT_BC1_RGBA_SRGB ||
        dst.format == RHI_FORMAT_BC3_UNORM || dst.format == RHI_FORMAT_BC3_SRGB ||
        dst.format == RHI_FORMAT_BC5_UNORM || dst.format == RHI_FORMAT_BC7_UNORM ||
        dst.format == RHI_FORMAT_BC7_SRGB);

    dst.extent = src.extent;
    dst.layer_count = src.layer_count;
    dst.level_count = src.level_count;

    pixel_layout src_layout = get_pixels_layout(
        src.format,
        src.extent.width,
        src.extent.height,
        src.layer_count,
        src.level_count);

    CMP_FORMAT src_format = get_cmp_format(src.format);
    CMP_FORMAT dst_format = get_cmp_format(dst.format);

    for (std::uint32_t layer = 0; layer < src.layer_count; ++layer)
    {
        for (std::uint32_t level = 0; level < src.level_count; ++level)
        {
            std::uint32_t width = std::max(src.extent.width >> level, 1u);
            std::uint32_t height = std::max(src.extent.height >> level, 1u);

            CMP_Texture src_texture = {
                .dwSize = sizeof(CMP_Texture),
                .dwWidth = width,
                .dwHeight = height,
                .dwPitch = 0,
                .format = src_format,
                .dwDataSize = static_cast<CMP_DWORD>(src_layout.level_size[level]),
                .pData = (CMP_BYTE*)(src.pixels.data() + src_layout.get_offset(layer, level)),
            };

            CMP_Texture dst_texture = {
                .dwSize = sizeof(CMP_Texture),
                .dwWidth = width,
                .dwHeight = height,
                .dwPitch = 0,
                .format = dst_format,
            };

            dst_texture.dwDataSize = CMP_CalculateBufferSize(&dst_texture);

            if (layer == 0 && level == 0)
            {
                std::size_t layer_size =
                    static_cast<std::size_t>(static_cast<double>(dst_texture.dwDataSize) * 1.3);
                dst.pixels.reserve(layer_size * src.layer_count);
            }

            std::size_t dst_offset = dst.pixels.size();
            dst.pixels.resize(dst_offset + dst_texture.dwDataSize);

            dst_texture.pData = reinterpret_cast<CMP_BYTE*>(dst.pixels.data() + dst_offset);

            CMP_CompressOptions options = {
                .dwSize = sizeof(CMP_CompressOptions),
                .fquality = 0.05f,
            };

            CMP_ERROR cmp_status =
                CMP_ConvertTexture(&src_texture, &dst_texture, &options, nullptr);
            if (cmp_status != CMP_OK)
            {
                return false;
            }
        }
    }

    return true;
}

bool texture_tool::generate_mipmaps(const texture_data& src, texture_data& dst)
{
    assert(src.level_count == 1);

    dst = {
        .format = src.format,
        .extent = src.extent,
        .layer_count = src.layer_count,
        .level_count = static_cast<std::uint32_t>(
                           std::floor(std::log2(std::max(src.extent.width, src.extent.height)))) +
                       1,
    };

    std::vector<std::size_t> src_offsets;
    pixel_layout src_layout = get_pixels_layout(
        src.format,
        src.extent.width,
        src.extent.height,
        src.layer_count,
        src.level_count);

    std::vector<std::size_t> dst_offsets;
    pixel_layout dst_layout = get_pixels_layout(
        dst.format,
        dst.extent.width,
        dst.extent.height,
        dst.layer_count,
        dst.level_count);
    dst.pixels.resize(dst_layout.total_size);

    const auto* src_pixels = reinterpret_cast<const std::uint8_t*>(src.pixels.data());
    auto* dst_pixels = reinterpret_cast<std::uint8_t*>(dst.pixels.data());

    rhi_format_size format_size = rhi_get_format_size(src.format);

    for (std::uint32_t layer = 0; layer < src.layer_count; ++layer)
    {
        std::memcpy(
            dst_pixels + dst_layout.get_offset(layer, 0),
            src_pixels + src_layout.get_offset(layer, 0),
            src_layout.level_size[0]);

        for (std::uint32_t level = 1; level < dst.level_count; ++level)
        {
            std::uint32_t width = std::max(src.extent.width >> (level - 1), 1u);
            std::uint32_t height = std::max(src.extent.height >> (level - 1), 1u);

            std::uint32_t next_width = std::max(src.extent.width >> level, 1u);
            std::uint32_t next_height = std::max(src.extent.height >> level, 1u);

            stbir_resize_uint8(
                dst_pixels + dst_layout.get_offset(layer, level - 1),
                static_cast<int>(width),
                static_cast<int>(height),
                0,
                dst_pixels + dst_layout.get_offset(layer, level),
                static_cast<int>(next_width),
                static_cast<int>(next_height),
                0,
                static_cast<int>(format_size.block_size));
        }
    }

    return true;
}
} // namespace violet