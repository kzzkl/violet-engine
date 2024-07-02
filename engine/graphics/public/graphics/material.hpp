#pragma once

#include "graphics/render_device.hpp"
#include "graphics/render_graph/rdg_pipeline.hpp"

namespace violet
{
class material
{
public:
    material();

    void add_pass(const rdg_render_pipeline& pipeline, const rhi_parameter_desc& parameter);
    const std::vector<rdg_render_pipeline>& get_passes() const noexcept { return m_passes; }

    rhi_parameter* get_parameter(std::size_t pass_index) const
    {
        return m_parameters[pass_index].get();
    }

private:
    std::vector<rdg_render_pipeline> m_passes;
    std::vector<rhi_ptr<rhi_parameter>> m_parameters;
};
} // namespace violet