#include "d3d12_common.hpp"

namespace violet::graphics::d3d12
{
DXGI_FORMAT d3d12_utility::convert_format(resource_format format)
{
    switch (format)
    {
    case RESOURCE_FORMAT_UNDEFINED:
        return DXGI_FORMAT_UNKNOWN;
    case RESOURCE_FORMAT_R8_UNORM:
        return DXGI_FORMAT_R8_UNORM;
    case RESOURCE_FORMAT_R8_UINT:
        return DXGI_FORMAT_R8_UINT;
    case RESOURCE_FORMAT_R8G8B8A8_UNORM:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case RESOURCE_FORMAT_B8G8R8A8_UNORM:
        return DXGI_FORMAT_B8G8R8A8_UNORM;
    case RESOURCE_FORMAT_R32G32B32A32_FLOAT:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case RESOURCE_FORMAT_R32G32B32A32_SINT:
        return DXGI_FORMAT_R32G32B32A32_SINT;
    case RESOURCE_FORMAT_R32G32B32A32_UINT:
        return DXGI_FORMAT_R32G32B32A32_UINT;
    case RESOURCE_FORMAT_D24_UNORM_S8_UINT:
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case RESOURCE_FORMAT_D32_FLOAT:
        return DXGI_FORMAT_D32_FLOAT;
    default:
        throw d3d12_exception("Invalid resource format.");
    };
}

resource_format d3d12_utility::convert_format(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_UNKNOWN:
        return RESOURCE_FORMAT_UNDEFINED;
    case DXGI_FORMAT_R8_UNORM:
        return RESOURCE_FORMAT_R8_UNORM;
    case DXGI_FORMAT_R8_UINT:
        return RESOURCE_FORMAT_R8_UINT;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return RESOURCE_FORMAT_R8G8B8A8_UNORM;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return RESOURCE_FORMAT_B8G8R8A8_UNORM;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        return RESOURCE_FORMAT_R32G32B32A32_FLOAT;
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return RESOURCE_FORMAT_R32G32B32A32_SINT;
    case DXGI_FORMAT_R32G32B32A32_UINT:
        return RESOURCE_FORMAT_R32G32B32A32_UINT;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        return RESOURCE_FORMAT_D24_UNORM_S8_UINT;
    case DXGI_FORMAT_D32_FLOAT:
        return RESOURCE_FORMAT_D32_FLOAT;
    default:
        throw d3d12_exception("Invalid resource format.");
    };
}

std::size_t d3d12_utility::element_size(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
        return 1;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        return 4;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
        return 16;
    default:
        throw d3d12_exception("Invalid resource format.");
    };
}
} // namespace violet::graphics::d3d12