#pragma once

#include "components/mesh_component.hpp"
#include "ecs/component.hpp"
#include "graphics/render_scene.hpp"

namespace violet
{
struct mesh_meta_component
{
    mesh_meta_component() = default;

    mesh_meta_component(const mesh_meta_component&) = delete;
    mesh_meta_component(mesh_meta_component&& other) noexcept
        : scene(other.scene),
          mesh(other.mesh)
    {
        std::swap(other.instances, instances);

        other.scene = nullptr;
        other.mesh = INVALID_RENDER_ID;
    }

    ~mesh_meta_component()
    {
        for (std::uint32_t instance_id : instances)
        {
            scene->remove_instance(instance_id);
        }

        if (mesh != INVALID_RENDER_ID)
        {
            scene->remove_mesh(mesh);
        }
    }

    mesh_meta_component& operator=(const mesh_meta_component&) = delete;
    mesh_meta_component& operator=(mesh_meta_component&& other) noexcept
    {
        scene = other.scene;
        mesh = other.mesh;
        std::swap(other.instances, instances);

        other.instances.clear();
        other.scene = nullptr;
        other.mesh = INVALID_RENDER_ID;

        return *this;
    }

    render_scene* scene{nullptr};

    render_id mesh{INVALID_RENDER_ID};
    std::vector<render_id> instances;
};

template <>
struct component_trait<mesh_meta_component>
{
    using main_component = mesh_component;
};
} // namespace violet