#include "graphics/render_graph/rdg_edge.hpp"

namespace violet
{
rdg_edge::rdg_edge(
    rdg_pass* src,
    rdg_pass_reference* src_reference,
    rdg_pass* dst,
    rdg_pass_reference* dst_reference)
    : m_src(src),
      m_src_reference(src_reference),
      m_dst(dst),
      m_dst_reference(dst_reference),
      m_operate(RDG_EDGE_OPERATE_DONT_CARE)
{
}
} // namespace violet