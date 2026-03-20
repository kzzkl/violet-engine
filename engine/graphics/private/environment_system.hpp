#pragma once

#include "core/engine.hpp"
#include "render_scene_manager.hpp"

namespace violet
{
class skybox;
class environment_system : public system
{
public:
    environment_system();

    bool initialize(const dictionary& config) override;

    void update(render_scene_manager& scene_manager);

    bool need_record() const noexcept
    {
        return !m_skybox_update_queue.empty() || !m_atmosphere_update_queue.empty();
    }

    void record(rhi_command* command);

private:
    void update_skybox(rhi_command* command, entity entity);
    void update_atmosphere(rhi_command* command, entity entity);

    std::uint32_t m_system_version{0};

    std::vector<entity> m_skybox_update_queue;
    std::vector<entity> m_atmosphere_update_queue;
};
} // namespace violet