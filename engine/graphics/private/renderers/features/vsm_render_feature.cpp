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
        .flags = RHI_BUFFER_STORAGE | RHI_BUFFER_HOST_VISIBLE,
    });
    device.set_name(m_lru_state.get(), "VSM LRU State");

    m_lru_buffer = device.create_buffer({
        .size = sizeof(std::uint32_t) * VSM_PHYSICAL_PAGE_COUNT * 2,
        .flags = RHI_BUFFER_STORAGE,
    });
    device.set_name(m_lru_buffer.get(), "VSM LRU");
}
} // namespace violet