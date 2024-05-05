#include "graphics/render_graph/edge.hpp"

namespace violet
{
edge::edge(pass* src, pass_reference* src_reference, pass* dst, pass_reference* dst_reference)
    : m_src(src),
      m_src_reference(src_reference),
      m_dst(dst),
      m_dst_reference(dst_reference),
      m_operate(EDGE_OPERATE_DONT_CARE)
{
}
} // namespace violet