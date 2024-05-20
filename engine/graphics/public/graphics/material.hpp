#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class material_layout
{
public:
    material_layout(render_graph* render_graph, const std::vector<rdg_pass*>& passes);

    render_graph* get_render_graph() const noexcept { return m_render_graph; }
    const std::vector<rdg_pass*>& get_passes() const noexcept { return m_passes; }

private:
    render_graph* m_render_graph;
    std::vector<rdg_pass*> m_passes;
};

class material
{
public:
    material(render_device* device, material_layout* layout);

    material_layout* get_layout() const noexcept { return m_layout; }
    rhi_parameter* get_parameter(std::size_t pass_index) const
    {
        return m_parameters[pass_index].get();
    }

private:
    std::vector<rhi_ptr<rhi_parameter>> m_parameters;
    material_layout* m_layout;
};
} // namespace violet