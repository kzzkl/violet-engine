#pragma once

#include "d3d12_common.hpp"

namespace violet::d3d12
{
class d3d12_image_loader
{
    struct load_result
    {
        d3d12_ptr<D3D12Resource> resource;
        d3d12_ptr<D3D12Resource> temporary;
    };

public:
    static load_result load(std::string_view file, D3D12GraphicsCommandList* command_list);
    static load_result load(
        const std::uint8_t* data,
        std::uint32_t width,
        std::uint32_t height,
        resource_format format,
        D3D12GraphicsCommandList* command_list);

    static load_result load_cube(
        const std::vector<std::string_view>& files,
        D3D12GraphicsCommandList* command_list);

private:
    static load_result load_dds(std::string_view file, D3D12GraphicsCommandList* command_list);
    static load_result load_other(std::string_view file, D3D12GraphicsCommandList* command_list);
};
} // namespace violet::d3d12