#pragma once

#include "graphics/geometry.hpp"
#include "graphics/material.hpp"
#include <limits>
#include <optional>

namespace violet
{
class mesh_loader
{
public:
    struct submesh_data
    {
        std::uint32_t vertex_offset;
        std::uint32_t index_offset;
        std::uint32_t index_count;
        std::uint32_t material;
    };

    struct mesh_data
    {
        std::uint32_t geometry;
        std::vector<submesh_data> submeshes;
    };

    struct skin_data
    {
        std::vector<std::uint32_t> bones; // index into nodes
    };

    struct node_data
    {
        std::uint32_t mesh;
        std::uint32_t skin;

        vec3f position{0.0f, 0.0f, 0.0f};
        vec4f rotation{0.0f, 0.0f, 0.0f, 1.0f};
        vec3f scale{1.0f, 1.0f, 1.0f};

        std::uint32_t parent{std::numeric_limits<std::uint32_t>::max()};
    };

    struct scene_data
    {
        std::vector<rhi_ptr<rhi_texture>> textures;
        std::vector<std::unique_ptr<geometry>> geometries;
        std::vector<std::unique_ptr<material>> materials;

        std::vector<mesh_data> meshes;
        std::vector<node_data> nodes;
    };

    virtual ~mesh_loader() = default;

    virtual std::optional<scene_data> load() = 0;
};
} // namespace violet