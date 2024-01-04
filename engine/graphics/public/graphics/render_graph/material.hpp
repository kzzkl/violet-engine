#pragma once

#include "graphics/render_graph/render_pass.hpp"
#include <unordered_map>

namespace violet
{
class material
{
public:
    material();
    material(const material&) = delete;
    virtual ~material();

    virtual std::vector<std::string> get_layout() const = 0;

    render_pipeline* get_pipeline(std::size_t index) const { return m_pipelines[index]; }
    rhi_parameter* get_parameter(std::size_t index) const { return m_parameters[index].get(); }

    std::size_t get_pipeline_count() const noexcept { return m_pipelines.size(); }

    material& operator=(const material&) = delete;

private:
    friend class render_graph;

    std::vector<render_pipeline*> m_pipelines;
    std::vector<rhi_ptr<rhi_parameter>> m_parameters;
};
} // namespace violet