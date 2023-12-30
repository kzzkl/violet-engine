#pragma once

#include "graphics/render_graph/render_node.hpp"

namespace violet
{
class barrier : public render_node
{
public:
    barrier();

    void set_pipeline_state(rhi_pipeline_stage_flags src_state, rhi_pipeline_stage_flags dst_state);
    void add_buffer_barrier(const rhi_buffer_barrier& barrier);
    void add_image_barrier(const rhi_image_barrier& barrier);

    virtual bool compile(compile_context& context) override;
    virtual void execute(execute_context& context) override;

private:
    rhi_pipeline_stage_flags m_src_state;
    rhi_pipeline_stage_flags m_dst_state;
    std::vector<rhi_buffer_barrier> m_buffer_barriers;
    std::vector<rhi_image_barrier> m_image_barriers;
};
} // namespace violet