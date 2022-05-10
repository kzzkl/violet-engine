#include "d3d12_common.hpp"

namespace ash::graphics::d3d12
{
DXGI_FORMAT to_d3d12_format(resource_format format)
{
    static const DXGI_FORMAT map[] = {
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_SINT,
        DXGI_FORMAT_R32G32B32A32_UINT,
        DXGI_FORMAT_D24_UNORM_S8_UINT};
    return map[static_cast<std::size_t>(format)];
}

resource_format to_ash_format(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_UNKNOWN:
        return resource_format::UNDEFINED;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return resource_format::R8G8B8A8_UNORM;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return resource_format::B8G8R8A8_UNORM;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        return resource_format::R32G32B32A32_FLOAT;
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return resource_format::R32G32B32A32_INT;
    case DXGI_FORMAT_R32G32B32A32_UINT:
        return resource_format::R32G32B32A32_UINT;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        return resource_format::D24_UNORM_S8_UINT;
    default:
        throw d3d12_exception("Invalid resource format.");
    };
}
} // namespace ash::graphics::d3d12