#include "vk_image_loader.hpp"
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace ash::graphics::vk
{
#define DDSD_CAPS 0x1
#define DDSD_HEIGHT 0x2
#define DDSD_WIDTH 0x4
#define DDSD_PITCH 0x8
#define DDSD_PIXELFORMAT 0x1000
#define DDSD_MIPMAPCOUNT 0x20000
#define DDSD_LINEARSIZE 0x80000
#define DDSD_DEPTH 0x800000

#define DDSCAPS_COMPLEX 0x8
#define DDSCAPS_MIPMAP 0x400000
#define DDSCAPS_TEXTURE 0x1000

#define DDSCAPS2_CUBEMAP 0x200
#define DDSCAPS2_CUBEMAP_POSITIVEX 0x400
#define DDSCAPS2_CUBEMAP_NEGATIVEX 0x800
#define DDSCAPS2_CUBEMAP_POSITIVEY 0x1000
#define DDSCAPS2_CUBEMAP_NEGATIVEY 0x2000
#define DDSCAPS2_CUBEMAP_POSITIVEZ 0x4000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ 0x8000
#define DDSCAPS2_VOLUME 0x200000

#define DDPF_ALPHAPIXELS 0x1
#define DDPF_ALPHA 0x2
#define DDPF_FOURCC 0x4
#define DDPF_RGB 0x40
#define DDPF_YUV 0x200
#define DDPF_LUMINANCE 0x20000

#define FOURCC_DXT1 0x31545844
#define FOURCC_DXT3 0x33545844
#define FOURCC_DXT5 0x35545844

struct DDS_PIXELFORMAT
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

struct DDS_HEADER
{
    std::uint32_t dwSize;
    std::uint32_t dwFlags;
    std::uint32_t dwHeight;
    std::uint32_t dwWidth;
    std::uint32_t dwPitchOrLinearSize;
    std::uint32_t dwDepth;
    std::uint32_t dwMipMapCount;
    std::uint32_t dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    std::uint32_t dwCaps;
    std::uint32_t dwCaps2;
    std::uint32_t dwCaps3;
    std::uint32_t dwCaps4;
    std::uint32_t dwReserved2;
};

enum class vk_dds_format
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

bool vk_image_loader::load(std::string_view file)
{
    if (file.ends_with(".dds"))
        return load_dds(file);
    else
        return load_other(file);
}

bool vk_image_loader::load_dds(std::string_view file)
{
    std::ifstream fin(file.data(), std::ios::binary);
    if (!fin.is_open())
        return false;

    char magic[4] = {};
    fin.read(magic, 4);
    if (std::strncmp(magic, "DDS ", 4) != 0)
        return false;

    DDS_HEADER header = {};
    fin.read(reinterpret_cast<char*>(&header), sizeof(DDS_HEADER));

    VkFormat vk_format;
    vk_dds_format dds_format;
    std::uint32_t channels;
    if (header.ddspf.dwFlags & DDPF_FOURCC)
    {
        switch (header.ddspf.dwFourCC)
        {
        case FOURCC_DXT1:
            vk_format = VK_FORMAT_R8G8B8A8_SRGB;
            dds_format = vk_dds_format::RGBA_EXT1;
            channels = 3;
            break;
        case FOURCC_DXT3:
            vk_format = VK_FORMAT_R8G8B8A8_SRGB;
            dds_format = vk_dds_format::RGBA_EXT3;
            channels = 4;
            break;
        case FOURCC_DXT5:
            vk_format = VK_FORMAT_R8G8B8A8_SRGB;
            dds_format = vk_dds_format::RGBA_EXT5;
            channels = 4;
            break;
        default:
            throw vk_exception("Invalid texture format.");
        }
    }
    else if (
        header.ddspf.dwRGBBitCount == 32 && header.ddspf.dwRBitMask == 0x00FF0000 &&
        header.ddspf.dwGBitMask == 0x0000FF00 && header.ddspf.dwBBitMask == 0x000000FF &&
        header.ddspf.dwABitMask == 0xFF000000)
    {
        vk_format = VK_FORMAT_B8G8R8A8_SRGB;
        dds_format = vk_dds_format::BGRA;
        channels = 4;
    }
    else if (
        header.ddspf.dwRGBBitCount == 32 && header.ddspf.dwRBitMask == 0x000000FF &&
        header.ddspf.dwGBitMask == 0x0000FF00 && header.ddspf.dwBBitMask == 0x00FF0000 &&
        header.ddspf.dwABitMask == 0xFF000000)
    {
        vk_format = VK_FORMAT_R8G8B8A8_SRGB;
        dds_format = vk_dds_format::RGBA;
        channels = 4;
    }
    else if (
        header.ddspf.dwRGBBitCount == 24 && header.ddspf.dwRBitMask == 0x000000FF &&
        header.ddspf.dwGBitMask == 0x0000FF00 && header.ddspf.dwBBitMask == 0x00FF0000)
    {
        vk_format = VK_FORMAT_R8G8B8_SRGB;
        dds_format = vk_dds_format::RGB;
        channels = 3;
    }
    else if (
        header.ddspf.dwRGBBitCount == 24 && header.ddspf.dwRBitMask == 0x00FF0000 &&
        header.ddspf.dwGBitMask == 0x0000FF00 && header.ddspf.dwBBitMask == 0x000000FF)
    {
        vk_format = VK_FORMAT_B8G8R8_SRGB;
        dds_format = vk_dds_format::BGR;
        channels = 3;
    }
    else if (header.ddspf.dwRGBBitCount == 8)
    {
        dds_format = vk_dds_format::LUMINANCE;
        channels = 1;
    }
    else
    {
        throw vk_exception("Invalid texture format.");
    }

    auto calculate_texture_size = [](std::uint32_t width,
                                     std::uint32_t height,
                                     std::uint32_t channels,
                                     vk_dds_format format) {
        if (format == vk_dds_format::RGBA_EXT1 || format == vk_dds_format::RGBA_EXT3 ||
            format == vk_dds_format::RGBA_EXT5)
            return ((width + 3) / 4) * ((height + 3) / 4) *
                   (format == vk_dds_format::RGBA_EXT1 ? 8 : 16);
        else
            return width * height * channels;
    };

    std::uint32_t mip_width = header.dwWidth;
    std::uint32_t mip_height = header.dwHeight;
    for (std::uint32_t i = 0; i < header.dwMipMapCount; ++i)
    {
        vk_image_data texture;
        texture.width = mip_width;
        texture.height = mip_height;
        texture.channels = channels;
        texture.format = vk_format;

        std::size_t size = calculate_texture_size(mip_width, mip_height, channels, dds_format);
        texture.pixels.resize(size);
        fin.read(texture.pixels.data(), size);

        mip_width = mip_width >> 1;
        mip_height = mip_height >> 1;

        m_mipmap.push_back(std::move(texture));
    }

    fin.close();

    return true;
}

bool vk_image_loader::load_other(std::string_view file)
{
    int width, height, channels;

    stbi_uc* pixels = stbi_load(file.data(), &width, &height, &channels, STBI_rgb_alpha);
    std::size_t image_size = width * height * 4;

    vk_image_data texture;
    texture.width = static_cast<std::uint32_t>(width);
    texture.height = static_cast<std::uint32_t>(height);
    texture.channels = static_cast<std::uint32_t>(channels);
    texture.format = VK_FORMAT_R8G8B8A8_SRGB;
    texture.pixels.resize(image_size);

    std::memcpy(texture.pixels.data(), pixels, image_size);

    stbi_image_free(pixels);

    m_mipmap.push_back(std::move(texture));

    return true;
}
} // namespace ash::graphics::vk