#include "vk_acceleration_structure.hpp"
#include "vk_command.hpp"
#include "vk_context.hpp"
#include "vk_resource.hpp"
#include "vk_staging_uploader.hpp"
#include "vk_utils.hpp"
#include "vk_vma.hpp"
#include <algorithm>

namespace violet::vk
{
namespace
{
VkDeviceAddress get_buffer_address(VkBuffer buffer, VkDevice device) noexcept
{
    VkBufferDeviceAddressInfoKHR info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = buffer,
    };

    return vkGetBufferDeviceAddress(device, &info);
}

VkAccelerationStructureTypeKHR map_as_type(rhi_acceleration_structure_type type)
{
    switch (type)
    {
    case RHI_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL:
        return VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    case RHI_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL:
        return VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    default:
        throw std::runtime_error("invalid acceleration structure type");
    }
}

VkBuildAccelerationStructureFlagsKHR map_as_flags(rhi_acceleration_structure_flags flags)
{
    VkBuildAccelerationStructureFlagsKHR result = 0;
    result |= (flags & RHI_ACCELERATION_STRUCTURE_ALLOW_UPDATE) ?
                  VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR :
                  0;
    result |= (flags & RHI_ACCELERATION_STRUCTURE_ALLOW_COMPACTION) ?
                  VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR :
                  0;
    result |= (flags & RHI_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE) ?
                  VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR :
                  0;
    result |= (flags & RHI_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD) ?
                  VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR :
                  0;
    return result;
}

void get_geometry_data(
    const rhi_acceleration_structure_desc::triangle_data& triangles,
    std::vector<VkAccelerationStructureGeometryKHR>& geometries,
    std::vector<VkAccelerationStructureBuildRangeInfoKHR>& build_range_infos,
    vk_context* context)
{
    VkBuffer vertex_buffer = static_cast<vk_buffer*>(triangles.vertex_buffer)->get_buffer();
    VkBuffer index_buffer = static_cast<vk_buffer*>(triangles.index_buffer)->get_buffer();

    VkAccelerationStructureGeometryTrianglesDataKHR data = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
        .vertexFormat = vk_utils::map_format(triangles.vertex_format),
        .vertexData =
            {
                .deviceAddress = get_buffer_address(vertex_buffer, context->get_device()) +
                                 triangles.vertex_buffer_offset,
            },
        .vertexStride = rhi_get_format_block_size(triangles.vertex_format).block_size,
        .maxVertex = triangles.vertex_count - 1,
        .indexType = vk_utils::get_index_type(triangles.index_size),
        .indexData =
            {
                .deviceAddress = get_buffer_address(index_buffer, context->get_device()) +
                                 triangles.index_buffer_offset,
            },
    };

    geometries.resize(triangles.submesh_count);
    build_range_infos.resize(triangles.submesh_count);

    for (std::size_t i = 0; i < triangles.submesh_count; ++i)
    {
        auto& geometry = geometries[i];
        geometry = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
            .geometry = {.triangles = data},
            .flags =
                VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR | VK_GEOMETRY_OPAQUE_BIT_KHR,
        };

        auto& build_range_info = build_range_infos[i];
        build_range_info = {
            .primitiveCount = triangles.submeshes[i].index_count / 3,
            .primitiveOffset = triangles.submeshes[i].index_offset * triangles.index_size,
            .firstVertex = triangles.submeshes[i].vertex_offset,
            .transformOffset = 0,
        };
    }
}

void get_geometry_data(
    const rhi_acceleration_structure_desc::instance_data& instances,
    std::vector<VkAccelerationStructureGeometryKHR>& geometries,
    std::vector<VkAccelerationStructureBuildRangeInfoKHR>& build_range_infos,
    VkBuffer& tlas_buffer,
    VmaAllocation& tlas_allocation,
    vk_context* context)
{
    std::vector<VkAccelerationStructureInstanceKHR> tlas_instances;
    tlas_instances.reserve(instances.instance_count);

    for (const auto& instance : std::span(instances.instances, instances.instance_count))
    {
        VkAccelerationStructureKHR blas =
            static_cast<vk_acceleration_structure*>(instance.blas)->get_acceleration_structure();

        VkAccelerationStructureDeviceAddressInfoKHR blas_address_info = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
            .accelerationStructure = blas,
        };

        VkDeviceAddress blas_address =
            vkGetAccelerationStructureDeviceAddressKHR(context->get_device(), &blas_address_info);

        VkAccelerationStructureInstanceKHR as_instance = {
            .instanceCustomIndex = instance.custom_index,
            .mask = 0xFF,
            .instanceShaderBindingTableRecordOffset = 0,
            .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV,
            .accelerationStructureReference = blas_address,
        };
        std::memcpy(&as_instance.transform, instance.transform, sizeof(as_instance.transform));

        tlas_instances.emplace_back(as_instance);
    }

    VkBufferCreateInfo tlas_buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = tlas_instances.size() * sizeof(VkAccelerationStructureInstanceKHR),
        .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    };

    std::tie(tlas_buffer, tlas_allocation) =
        vk_vma::create_buffer(tlas_buffer_info, 0, context->get_vma_allocator());

    auto* staging_uploader = context->get_staging_uploader();
    staging_uploader->upload(
        tlas_instances.data(),
        tlas_instances.size() * sizeof(VkAccelerationStructureInstanceKHR),
        tlas_buffer);

    VkAccelerationStructureGeometryInstancesDataKHR data = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
        .data = {.deviceAddress = get_buffer_address(tlas_buffer, context->get_device())},
    };

    geometries.resize(1);
    build_range_infos.resize(1);

    geometries[0] = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
        .geometry = {.instances = data},
    };

    build_range_infos[0] = {.primitiveCount = instances.instance_count};
}
} // namespace

vk_acceleration_structure::vk_acceleration_structure(
    const rhi_acceleration_structure_desc& desc,
    vk_context* context)
    : m_context(context)
{
    std::vector<VkAccelerationStructureGeometryKHR> geometries;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> build_range_infos;

    switch (desc.geometry_type)
    {
    case RHI_GEOMETRY_TYPE_TRIANGLES: {
        get_geometry_data(desc.triangles, geometries, build_range_infos, context);

        create_acceleration_structure(
            map_as_type(desc.type),
            geometries,
            build_range_infos,
            map_as_flags(desc.flags));
        break;
    }
    case RHI_GEOMETRY_TYPE_INSTANCES: {
        VkBuffer tlas_buffer;
        VmaAllocation tlas_allocation;

        get_geometry_data(
            desc.instances,
            geometries,
            build_range_infos,
            tlas_buffer,
            tlas_allocation,
            context);

        create_acceleration_structure(
            map_as_type(desc.type),
            geometries,
            build_range_infos,
            map_as_flags(desc.flags));

        vk_vma::destroy_buffer(tlas_buffer, tlas_allocation, context->get_vma_allocator());
        break;
    }
    default:
        break;
    }
}

vk_acceleration_structure::~vk_acceleration_structure()
{
    vkDestroyAccelerationStructureKHR(m_context->get_device(), m_acceleration_structure, nullptr);
    vk_vma::destroy_buffer(m_buffer, m_allocation, m_context->get_vma_allocator());
}

void vk_acceleration_structure::create_acceleration_structure(
    VkAccelerationStructureTypeKHR type,
    std::span<VkAccelerationStructureGeometryKHR> geometries,
    std::span<VkAccelerationStructureBuildRangeInfoKHR> build_range_infos,
    VkBuildAccelerationStructureFlagsKHR flags)
{
    VkAccelerationStructureBuildGeometryInfoKHR build_info = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = type,
        .flags = flags,
        .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .geometryCount = static_cast<std::uint32_t>(geometries.size()),
        .pGeometries = geometries.data(),
    };

    VkAccelerationStructureBuildSizesInfoKHR build_size = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
    };

    std::vector<std::uint32_t> max_primitive_count(build_range_infos.size());
    std::ranges::transform(
        build_range_infos,
        max_primitive_count.begin(),
        [](auto& build_range_info)
        {
            return build_range_info.primitiveCount;
        });

    vkGetAccelerationStructureBuildSizesKHR(
        m_context->get_device(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &build_info,
        max_primitive_count.data(),
        &build_size);

    VkDeviceSize scratch_size = vk_utils::align_up(
        build_size.buildScratchSize,
        m_context->get_acceleration_structure_properties()
            .minAccelerationStructureScratchOffsetAlignment);

    VkBufferCreateInfo scratch_buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = scratch_size,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
    };

    auto [scratch_buffer, scratch_allocation] =
        vk_vma::create_buffer(scratch_buffer_info, 0, m_context->get_vma_allocator());

    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = build_size.accelerationStructureSize,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
    };

    std::tie(m_buffer, m_allocation) =
        vk_vma::create_buffer(buffer_info, 0, m_context->get_vma_allocator());

    VkAccelerationStructureCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .buffer = m_buffer,
        .size = build_size.accelerationStructureSize,
        .type = type,
    };

    vk_check(vkCreateAccelerationStructureKHR(
        m_context->get_device(),
        &create_info,
        nullptr,
        &m_acceleration_structure));

    vk_command* command = m_context->get_graphics_queue()->allocate_command();

    build_info.dstAccelerationStructure = m_acceleration_structure;
    build_info.scratchData.deviceAddress =
        get_buffer_address(scratch_buffer, m_context->get_device());

    VkAccelerationStructureBuildRangeInfoKHR* build_range_infos_ptr = build_range_infos.data();
    vkCmdBuildAccelerationStructuresKHR(
        command->get_command_buffer(),
        1,
        &build_info,
        &build_range_infos_ptr);

    m_context->get_graphics_queue()->execute_sync(command);

    vk_vma::destroy_buffer(scratch_buffer, scratch_allocation, m_context->get_vma_allocator());
}
} // namespace violet::vk