#include "graphics/renderers/features/vsm_render_feature.hpp"
#include "virtual_shadow_map/vsm_common.hpp"

namespace violet
{
vsm_render_feature::vsm_render_feature()
{
    auto& device = render_device::instance();

    struct lru_state_data
    {
        std::uint32_t head;
        std::uint32_t tail;
    };

    m_lru_state = device.create_buffer({
        .size = sizeof(lru_state_data) * 2,
        .flags = RHI_BUFFER_STORAGE,
    });
    m_lru_state->set_name("VSM LRU State");

    m_lru_buffer = device.create_buffer({
        .size = sizeof(std::uint32_t) * VSM_PHYSICAL_PAGE_TABLE_PAGE_COUNT * 2,
        .flags = RHI_BUFFER_STORAGE,
    });
    m_lru_buffer->set_name("VSM LRU");
}

void vsm_render_feature::set_debug_info(bool enable)
{
    if (!enable)
    {
        m_debug_info = nullptr;
        return;
    }

    if (m_debug_info == nullptr)
    {
        auto& device = render_device::instance();

        m_debug_info = device.create_buffer({
            .size = sizeof(debug_info) * device.get_frame_resource_count(),
            .flags = RHI_BUFFER_STORAGE | RHI_BUFFER_HOST_VISIBLE | RHI_BUFFER_TRANSFER_DST,
        });
        m_debug_info->set_name("VSM Debug Info");
    }
}

vsm_render_feature::debug_info vsm_render_feature::get_debug_info() const
{
    if (m_debug_info == nullptr)
    {
        return {};
    }

    debug_info info = {};
    auto& device = render_device::instance();

    auto* mapping = static_cast<std::uint8_t*>(m_debug_info->get_buffer_pointer());
    std::memcpy(
        &info,
        mapping + (device.get_frame_resource_index() * sizeof(debug_info)),
        sizeof(debug_info));

    std::memset(
        mapping + (device.get_frame_resource_index() * sizeof(debug_info)),
        0,
        sizeof(debug_info));

    return info;
}
} // namespace violet