#pragma once

#include "core/engine.hpp"
#include "graphics/render_graph/render_graph.hpp"
#include "render_scene/render_scene_manager.hpp"

namespace violet
{
class skybox;
class environment_system : public system
{
public:
    environment_system();

    bool initialize(const dictionary& config) override;

    void update(render_scene_manager& scene_manager);

    void record(render_graph& graph);

private:
    void update_skybox(render_graph& graph, entity entity);
    void update_atmosphere(render_graph& graph, entity entity);

    std::uint32_t m_system_version{0};

    std::vector<entity> m_skybox_update_queue;
    std::vector<entity> m_atmosphere_update_queue;
};
} // namespace violet