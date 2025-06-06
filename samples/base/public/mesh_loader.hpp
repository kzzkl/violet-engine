#pragma once

#include "graphics/geometry.hpp"
#include "graphics/material.hpp"
#include "graphics/resources/texture.hpp"
#include <optional>

namespace violet
{
class mesh_loader
{
public:
    struct submesh_data
    {
        std::uint32_t submesh_index;
        std::int32_t material;
    };

    struct mesh_data
    {
        std::uint32_t geometry;
        std::vector<submesh_data> submeshes;
    };

    struct node_data
    {
        std::string name;

        std::int32_t mesh{-1};

        vec3f position{0.0f, 0.0f, 0.0f};
        vec4f rotation{0.0f, 0.0f, 0.0f, 1.0f};
        vec3f scale{1.0f, 1.0f, 1.0f};

        std::int32_t parent{-1};
    };

    struct scene_data
    {
        std::vector<std::unique_ptr<texture_2d>> textures;
        std::vector<std::unique_ptr<material>> materials;
        std::vector<std::unique_ptr<geometry>> geometries;

        std::vector<mesh_data> meshes;
        std::vector<node_data> nodes;
    };

    virtual ~mesh_loader() = default;

    virtual std::optional<scene_data> load() = 0;
};
} // namespace violet