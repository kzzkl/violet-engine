#include "d3d12_frame_resource.hpp"

namespace ash::graphics::d3d12
{
d3d12_frame_counter::d3d12_frame_counter() noexcept : m_frame_counter(0), m_frame_resousrce_count(0)
{
}

d3d12_frame_counter& d3d12_frame_counter::instance() noexcept
{
    static d3d12_frame_counter instance;
    return instance;
}

void d3d12_frame_counter::initialize(
    std::size_t frame_counter,
    std::size_t frame_resousrce_count) noexcept
{
    instance().m_frame_counter = frame_counter;
    instance().m_frame_resousrce_count = frame_resousrce_count;
}

void d3d12_frame_counter::tick() noexcept
{
    ++instance().m_frame_counter;
}
} // namespace ash::graphics::d3d12