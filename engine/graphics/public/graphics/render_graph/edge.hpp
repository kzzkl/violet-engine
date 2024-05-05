#pragma once

#include "graphics/render_graph/pass.hpp"

namespace violet
{
enum edge_operate
{
    EDGE_OPERATE_DONT_CARE,
    EDGE_OPERATE_CLEAR,
    EDGE_OPERATE_STORE
};

class edge
{
public:
    edge(pass* src, pass_reference* src_reference, pass* dst, pass_reference* dst_reference);

    void set_operate(edge_operate operate) noexcept { m_operate = operate; }
    edge_operate get_operate() const noexcept { return m_operate; }

    pass* get_src() const noexcept { return m_src; }
    pass_reference* get_src_reference() const noexcept { return m_src_reference; }

    pass* get_dst() const noexcept { return m_dst; }
    pass_reference* get_dst_reference() const noexcept { return m_dst_reference; }

private:
    pass* m_src;
    pass_reference* m_src_reference;

    pass* m_dst;
    pass_reference* m_dst_reference;
    
    edge_operate m_operate;
};
} // namespace violet