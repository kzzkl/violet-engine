#pragma once

#include "vk_common.hpp"
#include <span>

namespace violet::vk
{
class vk_context;

class vk_acceleration_structure : public rhi_acceleration_structure
{
public:
    vk_acceleration_structure(const rhi_acceleration_structure_desc& desc, vk_context* context);
    ~vk_acceleration_structure();

    VkAccelerationStructureKHR get_acceleration_structure() const noexcept
    {
        return m_acceleration_structure;
    }

private:
    void create_acceleration_structure(
        VkAccelerationStructureTypeKHR type,
        std::span<VkAccelerationStructureGeometryKHR> geometries,
        std::span<VkAccelerationStructureBuildRangeInfoKHR> build_range_infos,
        VkBuildAccelerationStructureFlagsKHR flags);

    VkAccelerationStructureKHR m_acceleration_structure;

    VkBuffer m_buffer;
    VmaAllocation m_allocation;

    vk_context* m_context{nullptr};
};
} // namespace violet::vk