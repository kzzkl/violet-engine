#pragma once

#include "graphics/resources/texture.hpp"
#include <optional>

namespace violet
{
class mesh_loader
{
public:
    struct mesh_data
    {
        std::uint32_t geometry;
        std::vector<std::uint32_t> submeshes;
        std::vector<std::uint32_t> materials;
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

    struct material_data
    {
        vec3f albedo;
        float roughness;
        float metallic;
        vec3f emissive;

        texture_2d* albedo_texture;
        texture_2d* roughness_metallic_texture;
        texture_2d* emissive_texture;
        texture_2d* normal_texture;
    };

    struct submesh_data
    {
        std::uint32_t vertex_offset;
        std::uint32_t index_offset;
        std::uint32_t index_count;
    };

    struct geometry_data
    {
        std::vector<vec3f> positions;
        std::vector<vec3f> normals;
        std::vector<vec4f> tangents;
        std::vector<vec2f> texcoords;
        std::vector<std::uint32_t> indexes;

        std::vector<submesh_data> submeshes;
    };

    struct scene_data
    {
        std::vector<std::unique_ptr<texture_2d>> textures;
        std::vector<material_data> materials;
        std::vector<geometry_data> geometries;

        std::vector<mesh_data> meshes;
        std::vector<node_data> nodes;
    };

    virtual ~mesh_loader() = default;

    virtual std::optional<scene_data> load(std::string_view path) = 0;
};
} // namespace violet