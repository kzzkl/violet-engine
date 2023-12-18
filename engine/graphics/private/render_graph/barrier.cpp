#include "graphics/render_graph/barrier.hpp"

namespace violet
{
barrier::barrier()
{
}

void barrier::set_pipeline_state(
    rhi_pipeline_stage_flags src_state,
    rhi_pipeline_stage_flags dst_state)
{
}

void barrier::add_buffer_barrier(const rhi_buffer_barrier& barrier)
{
    m_buffer_barriers.push_back(barrier);
}

void barrier::add_texture_barrier(const rhi_texture_barrier& barrier)
{
    m_texture_barriers.push_back(barrier);
}

bool barrier::compile(compile_context& context)
{
    return true;
}

void barrier::execute(execute_context& context)
{
    context.command->set_pipeline_barrier(
        m_src_state,
        m_dst_state,
        m_buffer_barriers.data(),
        m_buffer_barriers.size(),
        m_texture_barriers.data(),
        m_texture_barriers.size());
}
} // namespace violet