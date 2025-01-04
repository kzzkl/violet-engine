#pragma once

#include "core/engine.hpp"
#include "render_scene_manager.hpp"

namespace violet
{
class environment_system : public system
{
public:
    environment_system();

    bool initialize(const dictionary& config) override;

    void update(render_scene_manager& scene_manager);

private:
    std::uint32_t m_system_version{0};
};
} // namespace violet