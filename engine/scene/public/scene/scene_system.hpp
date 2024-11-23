#pragma once

#include "core/engine_system.hpp"
#include "math/math.hpp"
#include "scene/scene.hpp"

namespace violet
{
class scene_system : public engine_system
{
public:
    scene_system();

    virtual bool initialize(const dictionary& config) override;

    scene* create_scene(const std::string& name);

private:
    std::vector<std::unique_ptr<scene>> m_scenes;
};
} // namespace violet