#pragma once

#include "components/light_component.hpp"
#include "ecs/component.hpp"
#include "graphics/render_scene/render_scene.hpp"
#include "graphics/render_scene/render_scene_light.hpp"

namespace violet
{
struct light_component_meta
{
    light_component_meta() = default;
    light_component_meta(const light_component_meta&) = delete;
    light_component_meta(light_component_meta&& other) noexcept
    {
        scene = other.scene;
        id = other.id;

        other.scene = nullptr;
        other.id = INVALID_RENDER_ID;
    }

    ~light_component_meta()
    {
        if (id != INVALID_RENDER_ID)
        {
            scene->get_module<render_scene_light>().remove_light(id);
        }
    }

    light_component_meta& operator=(const light_component_meta&) = delete;
    light_component_meta& operator=(light_component_meta&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        scene = other.scene;
        id = other.id;

        other.scene = nullptr;
        other.id = INVALID_RENDER_ID;

        return *this;
    }

    render_scene* scene{nullptr};
    render_id id{INVALID_RENDER_ID};

    bool cast_shadow{false};
};

template <>
struct component_trait<light_component_meta>
{
    using main_component = light_component;
};
} // namespace violet