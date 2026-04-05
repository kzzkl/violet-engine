#pragma once

#include "graphics/cluster.hpp"
#include "graphics/resources/texture.hpp"

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

        std::int32_t albedo_texture{-1};
        std::int32_t roughness_metallic_texture{-1};
        std::int32_t emissive_texture{-1};
        std::int32_t normal_texture{-1};

        rhi_cull_mode cull_mode{RHI_CULL_MODE_BACK};
        float opacity_cutoff{0.0f};
    };

    struct submesh_data
    {
        std::uint32_t vertex_offset;
        std::uint32_t index_offset;
        std::uint32_t index_count;

        std::vector<cluster> clusters;
        std::vector<cluster_node> cluster_nodes;
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
        std::vector<texture_data> textures;
        std::vector<material_data> materials;
        std::vector<geometry_data> geometries;

        std::vector<mesh_data> meshes;
        std::vector<node_data> nodes;
    };

    static bool load(
        std::string_view path,
        scene_data& scene_data,
        bool generate_clusters,
        bool generate_mipmaps,
        bool compress_textures);

    static bool load(std::string_view path, scene_data& scene_data);
    static bool save(std::string_view path, const scene_data& scene_data);
};
} // namespace violet