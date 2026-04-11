#include "graphics/dds.hpp"
#include <fstream>

namespace violet
{
namespace
{
constexpr std::uint32_t MAGIC_NUMBER = 'D' << 0 | 'D' << 8 | 'S' << 16 | ' ' << 24;

enum ddpf : std::uint32_t
{
    DDPF_ALPHAPIXELS = 0x1,
    DDPF_ALPHA = 0x2,
    DDPF_FOURCC = 0x4,
    DDPF_RGB = 0x40,
    DDPF_YUV = 0x200,
    DDPF_LUMINANCE = 0x20000,
};

enum fourcc : std::uint32_t
{
    FOURCC_DXT1 = 'D' << 0 | 'X' << 8 | 'T' << 16 | '1' << 24,
    FOURCC_DXT2 = 'D' << 0 | 'X' << 8 | 'T' << 16 | '2' << 24,
    FOURCC_DXT3 = 'D' << 0 | 'X' << 8 | 'T' << 16 | '3' << 24,
    FOURCC_DXT4 = 'D' << 0 | 'X' << 8 | 'T' << 16 | '4' << 24,
    FOURCC_DXT5 = 'D' << 0 | 'X' << 8 | 'T' << 16 | '5' << 24,
    FOURCC_DX10 = 'D' << 0 | 'X' << 8 | '1' << 16 | '0' << 24,
    FOURCC_ATI1 = 'A' << 0 | 'T' << 8 | 'I' << 16 | '1' << 24,
    FOURCC_ATI2 = 'A' << 0 | 'T' << 8 | 'I' << 16 | '2' << 24,
};

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

enum ddsd : std::uint32_t
{
    DDSD_CAPS = 0x1,
    DDSD_HEIGHT = 0x2,
    DDSD_WIDTH = 0x4,
    DDSD_PITCH = 0x8,
    DDSD_PIXELFORMAT = 0x1000,
    DDSD_MIPMAPCOUNT = 0x20000,
    DDSD_LINEARSIZE = 0x80000,
    DDSD_DEPTH = 0x800000,
};

enum ddscaps : std::uint32_t
{
    DDSCAPS_COMPLEX = 0x8,
    DDSCAPS_MIPMAP = 0x400000,
    DDSCAPS_TEXTURE = 0x1000,
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

enum dxgi_format : std::uint32_t
{
    DXGI_FORMAT_UNKNOWN = 0,
    DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
    DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
    DXGI_FORMAT_R32G32B32A32_UINT = 3,
    DXGI_FORMAT_R32G32B32A32_SINT = 4,
    DXGI_FORMAT_R32G32B32_TYPELESS = 5,
    DXGI_FORMAT_R32G32B32_FLOAT = 6,
    DXGI_FORMAT_R32G32B32_UINT = 7,
    DXGI_FORMAT_R32G32B32_SINT = 8,
    DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
    DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
    DXGI_FORMAT_R16G16B16A16_UNORM = 11,
    DXGI_FORMAT_R16G16B16A16_UINT = 12,
    DXGI_FORMAT_R16G16B16A16_SNORM = 13,
    DXGI_FORMAT_R16G16B16A16_SINT = 14,
    DXGI_FORMAT_R32G32_TYPELESS = 15,
    DXGI_FORMAT_R32G32_FLOAT = 16,
    DXGI_FORMAT_R32G32_UINT = 17,
    DXGI_FORMAT_R32G32_SINT = 18,
    DXGI_FORMAT_R32G8X24_TYPELESS = 19,
    DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
    DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
    DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
    DXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
    DXGI_FORMAT_R10G10B10A2_UNORM = 24,
    DXGI_FORMAT_R10G10B10A2_UINT = 25,
    DXGI_FORMAT_R11G11B10_FLOAT = 26,
    DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
    DXGI_FORMAT_R8G8B8A8_UNORM = 28,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
    DXGI_FORMAT_R8G8B8A8_UINT = 30,
    DXGI_FORMAT_R8G8B8A8_SNORM = 31,
    DXGI_FORMAT_R8G8B8A8_SINT = 32,
    DXGI_FORMAT_R16G16_TYPELESS = 33,
    DXGI_FORMAT_R16G16_FLOAT = 34,
    DXGI_FORMAT_R16G16_UNORM = 35,
    DXGI_FORMAT_R16G16_UINT = 36,
    DXGI_FORMAT_R16G16_SNORM = 37,
    DXGI_FORMAT_R16G16_SINT = 38,
    DXGI_FORMAT_R32_TYPELESS = 39,
    DXGI_FORMAT_D32_FLOAT = 40,
    DXGI_FORMAT_R32_FLOAT = 41,
    DXGI_FORMAT_R32_UINT = 42,
    DXGI_FORMAT_R32_SINT = 43,
    DXGI_FORMAT_R24G8_TYPELESS = 44,
    DXGI_FORMAT_D24_UNORM_S8_UINT = 45,
    DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
    DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,
    DXGI_FORMAT_R8G8_TYPELESS = 48,
    DXGI_FORMAT_R8G8_UNORM = 49,
    DXGI_FORMAT_R8G8_UINT = 50,
    DXGI_FORMAT_R8G8_SNORM = 51,
    DXGI_FORMAT_R8G8_SINT = 52,
    DXGI_FORMAT_R16_TYPELESS = 53,
    DXGI_FORMAT_R16_FLOAT = 54,
    DXGI_FORMAT_D16_UNORM = 55,
    DXGI_FORMAT_R16_UNORM = 56,
    DXGI_FORMAT_R16_UINT = 57,
    DXGI_FORMAT_R16_SNORM = 58,
    DXGI_FORMAT_R16_SINT = 59,
    DXGI_FORMAT_R8_TYPELESS = 60,
    DXGI_FORMAT_R8_UNORM = 61,
    DXGI_FORMAT_R8_UINT = 62,
    DXGI_FORMAT_R8_SNORM = 63,
    DXGI_FORMAT_R8_SINT = 64,
    DXGI_FORMAT_A8_UNORM = 65,
    DXGI_FORMAT_R1_UNORM = 66,
    DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
    DXGI_FORMAT_R8G8_B8G8_UNORM = 68,
    DXGI_FORMAT_G8R8_G8B8_UNORM = 69,
    DXGI_FORMAT_BC1_TYPELESS = 70,
    DXGI_FORMAT_BC1_UNORM = 71,
    DXGI_FORMAT_BC1_UNORM_SRGB = 72,
    DXGI_FORMAT_BC2_TYPELESS = 73,
    DXGI_FORMAT_BC2_UNORM = 74,
    DXGI_FORMAT_BC2_UNORM_SRGB = 75,
    DXGI_FORMAT_BC3_TYPELESS = 76,
    DXGI_FORMAT_BC3_UNORM = 77,
    DXGI_FORMAT_BC3_UNORM_SRGB = 78,
    DXGI_FORMAT_BC4_TYPELESS = 79,
    DXGI_FORMAT_BC4_UNORM = 80,
    DXGI_FORMAT_BC4_SNORM = 81,
    DXGI_FORMAT_BC5_TYPELESS = 82,
    DXGI_FORMAT_BC5_UNORM = 83,
    DXGI_FORMAT_BC5_SNORM = 84,
    DXGI_FORMAT_B5G6R5_UNORM = 85,
    DXGI_FORMAT_B5G5R5A1_UNORM = 86,
    DXGI_FORMAT_B8G8R8A8_UNORM = 87,
    DXGI_FORMAT_B8G8R8X8_UNORM = 88,
    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
    DXGI_FORMAT_B8G8R8A8_TYPELESS = 90,
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
    DXGI_FORMAT_B8G8R8X8_TYPELESS = 92,
    DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
    DXGI_FORMAT_BC6H_TYPELESS = 94,
    DXGI_FORMAT_BC6H_UF16 = 95,
    DXGI_FORMAT_BC6H_SF16 = 96,
    DXGI_FORMAT_BC7_TYPELESS = 97,
    DXGI_FORMAT_BC7_UNORM = 98,
    DXGI_FORMAT_BC7_UNORM_SRGB = 99,
    DXGI_FORMAT_AYUV = 100,
    DXGI_FORMAT_Y410 = 101,
    DXGI_FORMAT_Y416 = 102,
    DXGI_FORMAT_NV12 = 103,
    DXGI_FORMAT_P010 = 104,
    DXGI_FORMAT_P016 = 105,
    DXGI_FORMAT_420_OPAQUE = 106,
    DXGI_FORMAT_YUY2 = 107,
    DXGI_FORMAT_Y210 = 108,
    DXGI_FORMAT_Y216 = 109,
    DXGI_FORMAT_NV11 = 110,
    DXGI_FORMAT_AI44 = 111,
    DXGI_FORMAT_IA44 = 112,
    DXGI_FORMAT_P8 = 113,
    DXGI_FORMAT_A8P8 = 114,
    DXGI_FORMAT_B4G4R4A4_UNORM = 115,
    DXGI_FORMAT_P208 = 130,
    DXGI_FORMAT_V208 = 131,
    DXGI_FORMAT_V408 = 132,
    DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE = 189,
    DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE = 190,
    DXGI_FORMAT_FORCE_UINT = 0xffffffff,
};

enum d3d10_resource_dimension
{
    D3D10_RESOURCE_DIMENSION_UNKNOWN = 0,
    D3D10_RESOURCE_DIMENSION_BUFFER = 1,
    D3D10_RESOURCE_DIMENSION_TEXTURE1D = 2,
    D3D10_RESOURCE_DIMENSION_TEXTURE2D = 3,
    D3D10_RESOURCE_DIMENSION_TEXTURE3D = 4
};

struct dds_header_dxt10
{
    dxgi_format dxgiFormat;
    d3d10_resource_dimension resourceDimension;
    std::uint32_t miscFlag;
    std::uint32_t arraySize;
    std::uint32_t miscFlags2;
};

dxgi_format get_dxgi_format(rhi_format format)
{
    switch (format)
    {
    case RHI_FORMAT_R8_UNORM:
        return DXGI_FORMAT_R8_UNORM;
    case RHI_FORMAT_R8G8_UNORM:
        return DXGI_FORMAT_R8G8_UNORM;
    case RHI_FORMAT_R8G8B8_UNORM:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case RHI_FORMAT_R8G8B8A8_UNORM:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case RHI_FORMAT_R8G8B8A8_SRGB:
        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case RHI_FORMAT_B8G8R8A8_UNORM:
        return DXGI_FORMAT_B8G8R8A8_UNORM;
    case RHI_FORMAT_B8G8R8A8_SRGB:
        return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    case RHI_FORMAT_R16G16B16A16_FLOAT:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case RHI_FORMAT_R32G32B32A32_FLOAT:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case RHI_FORMAT_BC1_RGB_UNORM:
    case RHI_FORMAT_BC1_RGBA_UNORM:
        return DXGI_FORMAT_BC1_UNORM;
    case RHI_FORMAT_BC1_RGB_SRGB:
    case RHI_FORMAT_BC1_RGBA_SRGB:
        return DXGI_FORMAT_BC1_UNORM_SRGB;
    case RHI_FORMAT_BC3_UNORM:
        return DXGI_FORMAT_BC3_UNORM;
    case RHI_FORMAT_BC3_SRGB:
        return DXGI_FORMAT_BC3_UNORM_SRGB;
    case RHI_FORMAT_BC5_UNORM:
        return DXGI_FORMAT_BC5_UNORM;
    case RHI_FORMAT_BC7_UNORM:
        return DXGI_FORMAT_BC7_UNORM;
    case RHI_FORMAT_BC7_SRGB:
        return DXGI_FORMAT_BC7_UNORM_SRGB;
    default:
        return DXGI_FORMAT_UNKNOWN;
    }
}

rhi_format get_rhi_format(dxgi_format format)
{
    switch (format)
    {
    case DXGI_FORMAT_R8_UNORM:
        return RHI_FORMAT_R8_UNORM;
    case DXGI_FORMAT_R8G8_UNORM:
        return RHI_FORMAT_R8G8_UNORM;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return RHI_FORMAT_R8G8B8A8_UNORM;
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return RHI_FORMAT_R8G8B8A8_SRGB;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return RHI_FORMAT_B8G8R8A8_UNORM;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return RHI_FORMAT_B8G8R8A8_SRGB;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        return RHI_FORMAT_R16G16B16A16_FLOAT;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        return RHI_FORMAT_R32G32B32A32_FLOAT;
    case DXGI_FORMAT_BC1_UNORM:
        return RHI_FORMAT_BC1_RGB_UNORM;
    case DXGI_FORMAT_BC1_UNORM_SRGB:
        return RHI_FORMAT_BC1_RGB_SRGB;
    case DXGI_FORMAT_BC3_UNORM:
        return RHI_FORMAT_BC3_UNORM;
    case DXGI_FORMAT_BC3_UNORM_SRGB:
        return RHI_FORMAT_BC3_SRGB;
    case DXGI_FORMAT_BC5_UNORM:
        return RHI_FORMAT_BC5_UNORM;
    case DXGI_FORMAT_BC7_UNORM:
        return RHI_FORMAT_BC7_UNORM;
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return RHI_FORMAT_BC7_SRGB;
    default:
        return RHI_FORMAT_UNDEFINED;
    }
}

bool is_compressed_format(rhi_format format)
{
    switch (format)
    {
    case RHI_FORMAT_BC1_RGB_UNORM:
    case RHI_FORMAT_BC1_RGBA_UNORM:
    case RHI_FORMAT_BC1_RGB_SRGB:
    case RHI_FORMAT_BC1_RGBA_SRGB:
    case RHI_FORMAT_BC3_UNORM:
    case RHI_FORMAT_BC3_SRGB:
    case RHI_FORMAT_BC5_UNORM:
    case RHI_FORMAT_BC7_UNORM:
    case RHI_FORMAT_BC7_SRGB:
        return true;
    default:
        return false;
    }
}
} // namespace

bool dds::save(std::string_view path, const texture_data& data)
{
    std::ofstream fout(std::string(path), std::ios::binary);
    if (!fout.is_open())
    {
        return false;
    }

    fout.write(reinterpret_cast<const char*>(&MAGIC_NUMBER), sizeof(std::uint32_t));

    dds_header header = {
        .dwSize = sizeof(dds_header),
        .dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT,
        .dwHeight = data.extent.height,
        .dwWidth = data.extent.width,
        .dwMipMapCount = data.level_count,
        .ddspf =
            {
                .dwSize = sizeof(dds_pixel_format),
                .dwFlags = DDPF_FOURCC,
                .dwFourCC = FOURCC_DX10,
            },
    };

    if (data.level_count > 1)
    {
        header.dwFlags |= DDSD_MIPMAPCOUNT;
    }

    rhi_format_size format_size = rhi_get_format_size(data.format);
    if (is_compressed_format(data.format))
    {
        header.dwPitchOrLinearSize =
            std::max(1u, ((data.extent.width + 3) / 4)) * format_size.block_size;
    }
    else
    {
        header.dwPitchOrLinearSize = data.extent.width * format_size.block_size;
    }

    header.dwCaps = DDSCAPS_TEXTURE;
    if (data.layer_count > 1)
    {
        header.dwCaps |= DDSCAPS_COMPLEX;
    }

    if (data.level_count > 1)
    {
        header.dwCaps |= DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    }

    fout.write(reinterpret_cast<const char*>(&header), sizeof(dds_header));

    dds_header_dxt10 header_dxt10 = {
        .dxgiFormat = get_dxgi_format(data.format),
        .resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D,
        .arraySize = data.layer_count,
    };
    fout.write(reinterpret_cast<const char*>(&header_dxt10), sizeof(dds_header_dxt10));

    fout.write(data.pixels.data(), static_cast<std::streamsize>(data.pixels.size()));

    return true;
}

bool dds::load(std::string_view path, texture_data& data)
{
    std::ifstream fin(std::string(path), std::ios::binary);
    if (!fin.is_open())
    {
        return false;
    }

    std::uint32_t magic_number;
    fin.read(reinterpret_cast<char*>(&magic_number), sizeof(std::uint32_t));

    if (magic_number != MAGIC_NUMBER)
    {
        return false;
    }

    dds_header header;
    fin.read(reinterpret_cast<char*>(&header), sizeof(dds_header));

    data.extent.width = header.dwWidth;
    data.extent.height = header.dwHeight;
    data.extent.depth = header.dwDepth > 0 ? header.dwDepth : 1;
    data.level_count = header.dwMipMapCount > 0 ? header.dwMipMapCount : 1;

    if (header.ddspf.dwFlags == DDPF_FOURCC && header.ddspf.dwFourCC == FOURCC_DX10)
    {
        dds_header_dxt10 header_dxt10;
        fin.read(reinterpret_cast<char*>(&header_dxt10), sizeof(dds_header_dxt10));

        data.format = get_rhi_format(header_dxt10.dxgiFormat);
        data.layer_count = header_dxt10.arraySize;
    }
    else
    {
        data.layer_count = 1;
    }

    std::streampos current = fin.tellg();

    fin.seekg(0, std::ios::end);
    std::streampos end = fin.tellg();

    std::streampos remaining = end - current;

    fin.seekg(current);

    data.pixels.resize(static_cast<std::size_t>(remaining));
    fin.read(data.pixels.data(), remaining);

    return true;
}
} // namespace violet