#pragma once

#include "components/mesh.hpp"
#include "graphics/render_context.hpp"

namespace violet
{
struct mesh_render_data
{
    mesh_render_data() = default;

    mesh_render_data(const mesh_render_data&) = delete;
    mesh_render_data(mesh_render_data&& other)
        : scene(other.scene),
          mesh(other.mesh)
    {
        std::swap(other.instances, instances);

        other.scene = nullptr;
        other.mesh = INVALID_RENDER_ID;
    }

    ~mesh_render_data()
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

    mesh_render_data& operator=(const mesh_render_data&) = delete;
    mesh_render_data& operator=(mesh_render_data&& other)
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
struct component_trait<mesh_render_data>
{
    using main_component = mesh;
};
} // namespace violet