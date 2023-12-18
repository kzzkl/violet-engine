#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet::sample
{
class pre_process_graph : public render_graph
{
public:
    pre_process_graph(renderer* renderer);

    void set_parameter(
        rhi_resource* ambient_map,
        rhi_sampler* ambient_sampler,
        rhi_resource* irradiance_map);

private:
    rhi_ptr<rhi_parameter> m_parameter;
};

class pbr_render_graph : public render_graph
{
public:
    pbr_render_graph(renderer* renderer);
};
} // namespace violet::sample