#include "graphics/render_graph/compute_pass.hpp"

namespace violet
{
compute_pass::compute_pass(std::string_view name, renderer* renderer)
    : render_node(name, renderer)
{
}

compute_pass::~compute_pass()
{
}

compute_pipeline* compute_pass::get_pipeline(std::string_view name) const
{
    for (auto& pipeline : m_pipelines)
    {
        if (pipeline->get_name() == name)
            return pipeline.get();
    }
    return nullptr;
}

bool compute_pass::compile()
{
    for (auto& pipeline : m_pipelines)
    {
        if (!pipeline->compile())
            return false;
    }
    return true;
}

void compute_pass::execute(rhi_render_command* command)
{
    for (auto& pipeline : m_pipelines)
        pipeline->execute(command);
}
} // namespace violet