#pragma once

#include "graphics/pipeline_parameter.hpp"
#include "graphics/render_graph/render_pipeline.hpp"

namespace violet
{
class material
{
public:
    void add_pipeline(render_pipeline* pipeline);
    void set_parameter(std::string_view name);

    const std::vector<std::pair<render_pipeline*, pipeline_parameter*>>& get_pipelines()
        const noexcept
    {
        return m_pipelines;
    }

private:
    struct parameter
    {
        std::size_t pipeline_index;
        std::size_t parameter_index;
    };

    std::vector<std::pair<render_pipeline*, pipeline_parameter*>> m_pipelines;
    std::vector<std::unique_ptr<pipeline_parameter>> m_parameters;
};
} // namespace violet