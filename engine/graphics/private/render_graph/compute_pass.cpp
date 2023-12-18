#include "graphics/render_graph/compute_pass.hpp"

namespace violet
{
compute_pass::compute_pass()
{
}

compute_pass::~compute_pass()
{
}

compute_pipeline* compute_pass::get_pipeline(std::string_view name) const
{
    return m_pipeline_map.at(name.data());
}

bool compute_pass::compile(compile_context& context)
{
    for (auto& pipeline : m_pipelines)
    {
        if (!pipeline->compile(context))
            return false;
    }
    return true;
}

void compute_pass::execute(execute_context& context)
{
    for (auto& pipeline : m_pipelines)
        pipeline->execute(context);
}
} // namespace violet