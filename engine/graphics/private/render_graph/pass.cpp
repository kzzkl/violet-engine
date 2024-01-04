#include "graphics/render_graph/pass.hpp"
#include <cassert>

namespace violet
{
setup_context::setup_context(
    const std::map<std::string, std::unique_ptr<render_resource>>& resources,
    std::map<std::string, std::pair<render_pipeline*, rhi_parameter_layout*>>& material_pipelines)
    : m_resources(resources),
      m_material_pipelines(material_pipelines)
{
}

render_resource* setup_context::read(std::string_view name)
{
    return m_resources.at(name.data()).get();
}

render_resource* setup_context::write(std::string_view name)
{
    return m_resources.at(name.data()).get();
}

void setup_context::register_material_pipeline(
    std::string_view name,
    render_pipeline* pipeline,
    rhi_parameter_layout* layout)
{
    assert(m_material_pipelines.find(name.data()) == m_material_pipelines.end());
    m_material_pipelines[name.data()] = std::make_pair(pipeline, layout);
}

execute_context::execute_context(
    rhi_render_command* command,
    rhi_parameter* light,
    const std::unordered_map<std::string, rhi_parameter*>& cameras)
    : m_command(command),
      m_light(light),
      m_cameras(cameras)
{
}

pass::pass(renderer* renderer, setup_context& context)
{
}

pass::~pass()
{
}
} // namespace violet