#pragma once

#include "graphics/render_scene/render_scene.hpp"

namespace violet
{
class render_scene_manager
{
public:
    render_scene_manager(vsm_manager* vsm_manager);

    render_scene* get_scene(std::uint32_t layer);

    template <typename T>
    void each_scene(T&& functor)
    {
        for (auto& scene : m_scenes)
        {
            functor(*scene);
        }
    }

private:
    std::vector<std::unique_ptr<render_scene>> m_scenes;

    vsm_manager* m_vsm_manager;
};
} // namespace violet