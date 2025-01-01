#pragma once

#include "components/light_component.hpp"
#include "ecs/component.hpp"
#include "graphics/render_scene.hpp"

namespace violet
{
struct light_meta_component
{
    light_meta_component() = default;
    light_meta_component(const light_meta_component&) = delete;
    light_meta_component(light_meta_component&& other) noexcept
    {
        scene = other.scene;
        id = other.id;

        other.scene = nullptr;
        other.id = INVALID_RENDER_ID;
    }

    ~light_meta_component()
    {
        if (id != INVALID_RENDER_ID)
        {
            scene->remove_light(id);
        }
    }

    light_meta_component& operator=(const light_meta_component&) = delete;
    light_meta_component& operator=(light_meta_component&& other) noexcept
    {
        scene = other.scene;
        id = other.id;

        other.scene = nullptr;
        other.id = INVALID_RENDER_ID;

        return *this;
    }

    render_scene* scene{nullptr};

    render_id id{INVALID_RENDER_ID};
};

template <>
struct component_trait<light_meta_component>
{
    using main_component = light_component;
};
} // namespace violet