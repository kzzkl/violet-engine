#pragma once

#include "components/mesh_component.hpp"
#include "ecs/component.hpp"
#include "graphics/render_scene/render_scene.hpp"
#include "graphics/render_scene/render_scene_mesh.hpp"
#include "graphics/render_scene/render_scene_sdf.hpp"

namespace violet
{
class mesh_component_meta
{
public:
    mesh_component_meta() = default;

    mesh_component_meta(const mesh_component_meta&) = delete;
    mesh_component_meta(mesh_component_meta&& other) noexcept
        : scene(other.scene),
          mesh(other.mesh)
    {
        std::swap(other.instances, instances);

        other.scene = nullptr;
        other.mesh = INVALID_RENDER_ID;
    }

    ~mesh_component_meta()
    {
        for (render_id instance_id : instances)
        {
            scene->get_module<render_scene_mesh>().remove_instance(instance_id);
        }

        if (mesh != INVALID_RENDER_ID)
        {
            scene->get_module<render_scene_mesh>().remove_mesh(mesh);
        }

        if (mesh_sdf != INVALID_RENDER_ID)
        {
            scene->get_module<render_scene_sdf>().remove_mesh(mesh_sdf);
        }
    }

    mesh_component_meta& operator=(const mesh_component_meta&) = delete;
    mesh_component_meta& operator=(mesh_component_meta&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        for (render_id instance_id : instances)
        {
            scene->get_module<render_scene_mesh>().remove_instance(instance_id);
        }

        if (mesh != INVALID_RENDER_ID)
        {
            scene->get_module<render_scene_mesh>().remove_mesh(mesh);
        }

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

    render_id mesh_sdf{INVALID_RENDER_ID};
};

template <>
struct component_trait<mesh_component_meta>
{
    using main_component = mesh_component;
};
} // namespace violet