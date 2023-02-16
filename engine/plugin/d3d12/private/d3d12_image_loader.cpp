#include "d3d12_image_loader.hpp"
#include "DDSTextureLoader12.h"
#include "d3d12_context.hpp"
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace violet::graphics::d3d12
{
d3d12_image_loader::load_result d3d12_image_loader::load(
    std::string_view file,
    D3D12GraphicsCommandList* command_list)
{
    if (file.ends_with(".dds"))
        return load_dds(file, command_list);
    else
        return load_other(file, command_list);
}

d3d12_image_loader::load_result d3d12_image_loader::load(
    const std::uint8_t* data,
    std::uint32_t width,
    std::uint32_t height,
    resource_format format,
    D3D12GraphicsCommandList* command_list)
{
    load_result result = {};

    DXGI_FORMAT texture_format = d3d12_utility::convert_format(format);
    std::size_t element_size = d3d12_utility::element_size(texture_format);

    auto device = d3d12_context::device();

    // Create default buffer.
    CD3DX12_HEAP_PROPERTIES default_heap_properties(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC default_desc = CD3DX12_RESOURCE_DESC::Tex2D(
        texture_format,
        static_cast<UINT>(width),
        static_cast<UINT>(height),
        1,
        1);
    throw_if_failed(device->CreateCommittedResource(
        &default_heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &default_desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&result.resource)));

    // Create upload buffer.
    UINT width_pitch = (width * element_size + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) &
                       ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);

    CD3DX12_HEAP_PROPERTIES upload_heap_properties(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC upload_desc = CD3DX12_RESOURCE_DESC::Buffer(height * width_pitch);
    throw_if_failed(device->CreateCommittedResource(
        &upload_heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &upload_desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&result.temporary)));

    void* mapped = nullptr;
    D3D12_RANGE range = {0, height * width_pitch};
    result.temporary->Map(0, &range, &mapped);
    for (std::size_t i = 0; i < height; ++i)
    {
        memcpy(
            static_cast<std::uint8_t*>(mapped) + i * width_pitch,
            data + i * width * element_size,
            width * element_size);
    }
    result.temporary->Unmap(0, &range);

    // Copy data to default buffer.
    D3D12_TEXTURE_COPY_LOCATION source_location = {};
    source_location.pResource = result.temporary.Get();
    source_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    source_location.PlacedFootprint.Footprint.Format = texture_format;
    source_location.PlacedFootprint.Footprint.Width = static_cast<UINT>(width);
    source_location.PlacedFootprint.Footprint.Height = static_cast<UINT>(height);
    source_location.PlacedFootprint.Footprint.Depth = 1;
    source_location.PlacedFootprint.Footprint.RowPitch = width_pitch;

    D3D12_TEXTURE_COPY_LOCATION target_location = {};
    target_location.pResource = result.resource.Get();
    target_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    target_location.SubresourceIndex = 0;

    command_list->CopyTextureRegion(&target_location, 0, 0, 0, &source_location, nullptr);

    CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
        result.resource.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    command_list->ResourceBarrier(1, &transition);

    return result;
}

d3d12_image_loader::load_result d3d12_image_loader::load_cube(
    const std::vector<std::string_view>& files,
    D3D12GraphicsCommandList* command_list)
{
    VIOLET_D3D12_ASSERT(files.size() == 6);

    load_result result = {};

    int width, height, channels;
    std::vector<stbi_uc*> pixels(6);
    std::vector<D3D12_SUBRESOURCE_DATA> subresources(6);
    for (std::size_t i = 0; i < 6; ++i)
    {
        stbi_uc* data = stbi_load(files[i].data(), &width, &height, &channels, STBI_rgb_alpha);
        VIOLET_D3D12_ASSERT(data != nullptr);
        pixels[i] = data;
        subresources[i].pData = data;
        subresources[i].RowPitch = width * 4;
        subresources[i].SlicePitch = 0;
    }

    auto device = d3d12_context::device();

    // Create default buffer.
    CD3DX12_RESOURCE_DESC default_desc =
        CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 6, 1);
    CD3DX12_HEAP_PROPERTIES default_heap_properties(D3D12_HEAP_TYPE_DEFAULT);
    throw_if_failed(device->CreateCommittedResource(
        &default_heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &default_desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&result.resource)));

    // Create upload buffer.
    const UINT64 upload_resource_size = GetRequiredIntermediateSize(
        result.resource.Get(),
        0,
        static_cast<UINT>(subresources.size()));
    CD3DX12_HEAP_PROPERTIES heap_properties(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(upload_resource_size);
    throw_if_failed(d3d12_context::device()->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&result.temporary)));

    UpdateSubresources(
        command_list,
        result.resource.Get(),
        result.temporary.Get(),
        0,
        0,
        static_cast<UINT>(subresources.size()),
        subresources.data());

    CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
        result.resource.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    command_list->ResourceBarrier(1, &transition);

    for (stbi_uc* data : pixels)
        stbi_image_free(data);

    return result;
}

d3d12_image_loader::load_result d3d12_image_loader::load_dds(
    std::string_view file,
    D3D12GraphicsCommandList* command_list)
{
    std::ifstream fin(file.data(), std::ios::in | std::ios::binary);
    if (!fin)
        throw d3d12_exception("Unable to open texture file.");

    load_result result = {};

    std::vector<uint8_t> dds_data(fin.seekg(0, std::ios::end).tellg());
    fin.seekg(0, std::ios::beg).read((char*)dds_data.data(), dds_data.size());
    fin.close();

    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    throw_if_failed(DirectX::LoadDDSTextureFromMemory(
        d3d12_context::device(),
        dds_data.data(),
        dds_data.size(),
        &result.resource,
        subresources));

    const UINT64 upload_resource_size = GetRequiredIntermediateSize(
        result.resource.Get(),
        0,
        static_cast<UINT>(subresources.size()));

    CD3DX12_HEAP_PROPERTIES heap_properties(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(upload_resource_size);
    throw_if_failed(d3d12_context::device()->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&result.temporary)));

    UpdateSubresources(
        command_list,
        result.resource.Get(),
        result.temporary.Get(),
        0,
        0,
        static_cast<UINT>(subresources.size()),
        subresources.data());

    CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
        result.resource.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    command_list->ResourceBarrier(1, &transition);

    return result;
}

d3d12_image_loader::load_result d3d12_image_loader::load_other(
    std::string_view file,
    D3D12GraphicsCommandList* command_list)
{
    int width, height, channels;

    stbi_uc* stb_pixels = stbi_load(file.data(), &width, &height, &channels, STBI_rgb_alpha);
    std::size_t image_size = width * height * 4;

    std::vector<std::uint8_t> pixels(image_size);
    std::memcpy(pixels.data(), stb_pixels, image_size);

    stbi_image_free(stb_pixels);

    return load(pixels.data(), width, height, RESOURCE_FORMAT_R8G8B8A8_UNORM, command_list);
}
} // namespace violet::graphics::d3d12