#include "graphics/renderers/features/cluster_render_feature.hpp"
#include "graphics/graphics_config.hpp"

namespace violet
{
std::uint32_t cluster_render_feature::get_max_clusters() const noexcept
{
    return graphics_config::get_max_candidate_clusters() - get_max_cluster_nodes();
}

std::uint32_t cluster_render_feature::get_max_cluster_nodes() const noexcept
{
    return graphics_config::get_max_candidate_clusters() / 16;
}

void cluster_render_feature::on_enable()
{
    // x: cluster or cluster node index
    // y: mesh index
    m_cluster_queue = render_device::instance().create_buffer({
        .size = graphics_config::get_max_candidate_clusters() * sizeof(vec2u),
        .flags = RHI_BUFFER_STORAGE,
    });

    // x: cluster queue offset
    // y: cluster node queue offset
    // z: cluster root count
    m_cluster_queue_state = render_device::instance().create_buffer({
        .size = sizeof(vec4u),
        .flags = RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST,
    });

    m_frame = 0;
}
} // namespace violet