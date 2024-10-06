#pragma once

#include "graphics/render_device.hpp"
#include "graphics/render_graph/rdg_pipeline.hpp"

namespace violet
{
class material
{
public:
    struct pass_info
    {
        rdg_render_pipeline pipeline;
        rhi_ptr<rhi_parameter> parameter;
    };

public:
    material();
    virtual ~material() = default;

    const std::vector<pass_info>& get_passes() const noexcept
    {
        return m_passes;
    }

protected:
    void add_pass(const rdg_render_pipeline& pipeline, const rhi_parameter_desc& parameter);
    pass_info& get_pass(std::size_t pass_index)
    {
        return m_passes[pass_index];
    }

private:
    std::vector<pass_info> m_passes;
};
} // namespace violet