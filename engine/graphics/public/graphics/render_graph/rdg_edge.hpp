#pragma once

#include "graphics/render_graph/rdg_pass.hpp"

namespace violet
{
enum rdg_edge_operate
{
    RDG_EDGE_OPERATE_DONT_CARE,
    RDG_EDGE_OPERATE_CLEAR,
    RDG_EDGE_OPERATE_STORE
};

class rdg_edge
{
public:
    rdg_edge(
        rdg_pass* src,
        rdg_pass_reference* src_reference,
        rdg_pass* dst,
        rdg_pass_reference* dst_reference);

    void set_operate(rdg_edge_operate operate) noexcept { m_operate = operate; }
    rdg_edge_operate get_operate() const noexcept { return m_operate; }

    rdg_pass* get_src() const noexcept { return m_src; }
    rdg_pass_reference* get_src_reference() const noexcept { return m_src_reference; }

    rdg_pass* get_dst() const noexcept { return m_dst; }
    rdg_pass_reference* get_dst_reference() const noexcept { return m_dst_reference; }

private:
    rdg_pass* m_src;
    rdg_pass_reference* m_src_reference;

    rdg_pass* m_dst;
    rdg_pass_reference* m_dst_reference;

    rdg_edge_operate m_operate;
};
} // namespace violet