#include "sample/mesh_loader.hpp"
#include "assimp_loader.hpp"
#include "common/log.hpp"
#include "gltf_loader.hpp"
#include "tools/geometry_tool.hpp"
#include "tools/texture_tool.hpp"
#include <filesystem>
#include <fstream>

namespace violet
{
namespace
{
constexpr std::uint32_t MAGIC_NUMBER = 0xFFFFFFFF;

enum block_type : std::uint32_t
{
    BLOCK_NODE = 0,
    BLOCK_GEOMETRY = 1,
    BLOCK_MATERIAL = 2,
    BLOCK_TEXTURE = 3,
};

enum geometry_flag
{
    GEOMETRY_HAS_NORMAL = 1 << 0,
    GEOMETRY_HAS_TANGENT = 1 << 1,
    GEOMETRY_HAS_TEXCOORD = 1 << 2,
};

template <typename T>
void read(std::ifstream& fin, T& value)
{
    fin.read(reinterpret_cast<char*>(&value), sizeof(T));
}

template <typename T>
void read(std::ifstream& fin, T* data, std::uint32_t count)
{
    fin.read(reinterpret_cast<char*>(data), static_cast<std::streamsize>(sizeof(T) * count));
}

template <typename T>
void write(std::ofstream& fout, const T& value)
{
    fout.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template <typename T>
void write(std::ofstream& fout, const T* data, std::uint32_t count)
{
    fout.write(
        reinterpret_cast<const char*>(data),
        static_cast<std::streamsize>(sizeof(T) * count));
}

bool read_node(std::ifstream& fin, mesh_loader::scene_data& scene_data)
{
    std::uint32_t node_count;
    read(fin, node_count);
    scene_data.nodes.resize(node_count);

    for (auto& node : scene_data.nodes)
    {
        std::uint32_t name_length;
        read(fin, name_length);

        node.name.resize(name_length);
        read(fin, node.name.data(), name_length);

        read(fin, node.position);
        read(fin, node.rotation);
        read(fin, node.scale);
        read(fin, node.parent);
        read(fin, node.mesh);
    }

    std::uint32_t mesh_count;
    read(fin, mesh_count);
    scene_data.meshes.resize(mesh_count);

    for (auto& mesh : scene_data.meshes)
    {
        read(fin, mesh.geometry);

        std::uint32_t submesh_count;
        read(fin, submesh_count);

        mesh.submeshes.resize(submesh_count);
        mesh.materials.resize(submesh_count);

        for (std::uint32_t i = 0; i < submesh_count; ++i)
        {
            read(fin, mesh.submeshes[i]);
            read(fin, mesh.materials[i]);
        }
    }

    return true;
}

bool write_node(std::ofstream& fout, const mesh_loader::scene_data& scene_data)
{
    auto node_count = static_cast<std::uint32_t>(scene_data.nodes.size());
    write(fout, node_count);

    for (const auto& node : scene_data.nodes)
    {
        auto name_length = static_cast<std::uint32_t>(node.name.size());
        write(fout, name_length);
        write(fout, node.name.data(), name_length);

        write(fout, node.position);
        write(fout, node.rotation);
        write(fout, node.scale);
        write(fout, node.parent);
        write(fout, node.mesh);
    }

    auto mesh_count = static_cast<std::uint32_t>(scene_data.meshes.size());
    write(fout, mesh_count);

    for (const auto& mesh : scene_data.meshes)
    {
        write(fout, mesh.geometry);

        auto submesh_count = static_cast<std::uint32_t>(mesh.submeshes.size());
        write(fout, submesh_count);

        for (std::uint32_t i = 0; i < submesh_count; ++i)
        {
            write(fout, mesh.submeshes[i]);
            write(fout, mesh.materials[i]);
        }
    }

    return true;
}

bool read_geometry(std::ifstream& fin, mesh_loader::scene_data& scene_data)
{
    std::uint32_t geometry_count;
    read(fin, geometry_count);
    scene_data.geometries.resize(geometry_count);

    for (auto& geometry : scene_data.geometries)
    {
        std::uint32_t vertex_count;
        read(fin, vertex_count);

        std::uint32_t index_count;
        read(fin, index_count);

        std::uint32_t flags;
        read(fin, flags);

        geometry.positions.resize(vertex_count);
        read(fin, geometry.positions.data(), vertex_count);

        geometry.indexes.resize(index_count);
        read(fin, geometry.indexes.data(), index_count);

        if (flags & GEOMETRY_HAS_NORMAL)
        {
            geometry.normals.resize(vertex_count);
            read(fin, geometry.normals.data(), vertex_count);
        }

        if (flags & GEOMETRY_HAS_TANGENT)
        {
            geometry.tangents.resize(vertex_count);
            read(fin, geometry.tangents.data(), vertex_count);
        }

        if (flags & GEOMETRY_HAS_TEXCOORD)
        {
            geometry.texcoords.resize(vertex_count);
            read(fin, geometry.texcoords.data(), vertex_count);
        }

        std::uint32_t submesh_count;
        read(fin, submesh_count);
        geometry.submeshes.resize(submesh_count);

        for (auto& submesh : geometry.submeshes)
        {
            std::uint32_t submesh_type;
            read(fin, submesh_type);

            if (submesh_type == 0)
            {
                read(fin, submesh.vertex_offset);
                read(fin, submesh.index_offset);
                read(fin, submesh.index_count);
            }
            else
            {
                std::uint32_t cluster_count;
                read(fin, cluster_count);
                submesh.clusters.resize(cluster_count);

                for (auto& cluster : submesh.clusters)
                {
                    read(fin, cluster.index_offset);
                    read(fin, cluster.index_count);
                    read(fin, cluster.bounding_sphere);
                    read(fin, cluster.lod_bounds);
                    read(fin, cluster.lod_error);
                    read(fin, cluster.parent_lod_bounds);
                    read(fin, cluster.parent_lod_error);
                    read(fin, cluster.lod);
                }

                std::uint32_t cluster_node_count;
                read(fin, cluster_node_count);
                submesh.cluster_nodes.resize(cluster_node_count);

                for (auto& cluster_node : submesh.cluster_nodes)
                {
                    read(fin, cluster_node.bounding_sphere);
                    read(fin, cluster_node.lod_bounds);
                    read(fin, cluster_node.min_lod_error);
                    read(fin, cluster_node.max_parent_lod_error);

                    std::uint32_t is_leaf;
                    read(fin, is_leaf);
                    cluster_node.is_leaf = is_leaf != 0;

                    read(fin, cluster_node.depth);
                    read(fin, cluster_node.child_offset);
                    read(fin, cluster_node.child_count);
                }
            }
        }
    }

    return true;
}

bool write_geometry(std::ofstream& fout, const mesh_loader::scene_data& scene_data)
{
    auto geometry_count = static_cast<std::uint32_t>(scene_data.geometries.size());
    write(fout, geometry_count);

    for (const auto& geometry : scene_data.geometries)
    {
        auto vertex_count = static_cast<std::uint32_t>(geometry.positions.size());
        write(fout, vertex_count);

        auto index_count = static_cast<std::uint32_t>(geometry.indexes.size());
        write(fout, index_count);

        std::uint32_t flags = 0;
        flags |= geometry.normals.empty() ? 0 : GEOMETRY_HAS_NORMAL;
        flags |= geometry.tangents.empty() ? 0 : GEOMETRY_HAS_TANGENT;
        flags |= geometry.texcoords.empty() ? 0 : GEOMETRY_HAS_TEXCOORD;
        write(fout, flags);

        write(fout, geometry.positions.data(), vertex_count);
        write(fout, geometry.indexes.data(), index_count);

        if (!geometry.normals.empty())
        {
            write(fout, geometry.normals.data(), vertex_count);
        }

        if (!geometry.tangents.empty())
        {
            write(fout, geometry.tangents.data(), vertex_count);
        }

        if (!geometry.texcoords.empty())
        {
            write(fout, geometry.texcoords.data(), vertex_count);
        }

        auto submesh_count = static_cast<std::uint32_t>(geometry.submeshes.size());
        write(fout, submesh_count);

        for (const auto& submesh : geometry.submeshes)
        {
            std::uint32_t submesh_type = submesh.clusters.empty() ? 0 : 1;
            write(fout, submesh_type);

            if (submesh.clusters.empty())
            {
                write(fout, submesh.vertex_offset);
                write(fout, submesh.index_offset);
                write(fout, submesh.index_count);
            }
            else
            {
                auto cluster_count = static_cast<std::uint32_t>(submesh.clusters.size());
                write(fout, cluster_count);

                for (const auto& cluster : submesh.clusters)
                {
                    write(fout, cluster.index_offset);
                    write(fout, cluster.index_count);
                    write(fout, cluster.bounding_sphere);
                    write(fout, cluster.lod_bounds);
                    write(fout, cluster.lod_error);
                    write(fout, cluster.parent_lod_bounds);
                    write(fout, cluster.parent_lod_error);
                    write(fout, cluster.lod);
                }

                auto cluster_node_count = static_cast<std::uint32_t>(submesh.cluster_nodes.size());
                write(fout, cluster_node_count);

                for (const auto& cluster_node : submesh.cluster_nodes)
                {
                    write(fout, cluster_node.bounding_sphere);
                    write(fout, cluster_node.lod_bounds);
                    write(fout, cluster_node.min_lod_error);
                    write(fout, cluster_node.max_parent_lod_error);
                    write(fout, static_cast<std::uint32_t>(cluster_node.is_leaf));
                    write(fout, cluster_node.depth);
                    write(fout, cluster_node.child_offset);
                    write(fout, cluster_node.child_count);
                }
            }
        }
    }

    return true;
}

bool read_material(std::ifstream& fin, mesh_loader::scene_data& scene_data)
{
    std::uint32_t material_count;
    read(fin, material_count);
    scene_data.materials.resize(material_count);

    for (auto& material : scene_data.materials)
    {
        read(fin, material.albedo);
        read(fin, material.roughness);
        read(fin, material.metallic);
        read(fin, material.emissive);

        read(fin, material.albedo_texture);
        read(fin, material.roughness_metallic_texture);
        read(fin, material.emissive_texture);
        read(fin, material.normal_texture);

        std::uint32_t cull_mode;
        read(fin, cull_mode);
        material.cull_mode = static_cast<rhi_cull_mode>(cull_mode);

        read(fin, material.opacity_cutoff);
    }

    return true;
}

bool write_material(std::ofstream& fout, const mesh_loader::scene_data& scene_data)
{
    auto material_count = static_cast<std::uint32_t>(scene_data.materials.size());
    write(fout, material_count);

    for (const auto& material : scene_data.materials)
    {
        write(fout, material.albedo);
        write(fout, material.roughness);
        write(fout, material.metallic);
        write(fout, material.emissive);

        write(fout, material.albedo_texture);
        write(fout, material.roughness_metallic_texture);
        write(fout, material.emissive_texture);
        write(fout, material.normal_texture);

        write(fout, static_cast<std::uint32_t>(material.cull_mode));
        write(fout, material.opacity_cutoff);
    }

    return true;
}

bool read_texture(std::ifstream& fin, mesh_loader::scene_data& scene_data)
{
    std::uint32_t texture_count;
    read(fin, texture_count);
    scene_data.textures.resize(texture_count);

    for (auto& texture : scene_data.textures)
    {
        std::uint32_t format;
        read(fin, format);
        texture.format = static_cast<rhi_format>(format);

        read(fin, texture.extent.width);
        read(fin, texture.extent.height);
        read(fin, texture.layer_count);
        read(fin, texture.level_count);

        std::uint32_t pixel_size;
        read(fin, pixel_size);

        texture.pixels.resize(pixel_size);
        read(fin, texture.pixels.data(), static_cast<std::streamsize>(pixel_size));
    }

    return true;
}

bool write_texture(std::ofstream& fout, const mesh_loader::scene_data& scene_data)
{
    auto texture_count = static_cast<std::uint32_t>(scene_data.textures.size());
    write(fout, texture_count);

    for (const auto& texture : scene_data.textures)
    {
        write(fout, static_cast<std::uint32_t>(texture.format));
        write(fout, texture.extent.width);
        write(fout, texture.extent.height);
        write(fout, texture.layer_count);
        write(fout, texture.level_count);

        auto pixel_size = static_cast<std::uint32_t>(texture.pixels.size());
        write(fout, pixel_size);
        write(fout, texture.pixels.data(), pixel_size);
    }

    return true;
}

bool mesh_loader_generate_clusters(mesh_loader::scene_data& scene_data)
{
    std::vector<std::thread> threads(std::thread::hardware_concurrency());

    std::atomic<std::uint32_t> current{0};
    std::atomic<std::uint32_t> total{0};

    for (auto& thread : threads)
    {
        thread = std::thread(
            [&]()
            {
                while (true)
                {
                    std::uint32_t index = current.fetch_add(1);

                    if (index >= scene_data.geometries.size())
                    {
                        break;
                    }

                    auto& geometry_data = scene_data.geometries[index];

                    geometry_tool::cluster_input input = {
                        .positions = geometry_data.positions,
                        .normals = geometry_data.normals,
                        .tangents = geometry_data.tangents,
                        .texcoords = geometry_data.texcoords,
                        .indexes = geometry_data.indexes,
                    };

                    for (const auto& submesh_data : geometry_data.submeshes)
                    {
                        input.submeshes.push_back({
                            .vertex_offset = submesh_data.vertex_offset,
                            .index_offset = submesh_data.index_offset,
                            .index_count = submesh_data.index_count,
                        });
                    }

                    geometry_tool::cluster_output output = geometry_tool::generate_clusters(input);
                    geometry_data.positions = std::move(output.positions);
                    geometry_data.normals = std::move(output.normals);
                    geometry_data.tangents = std::move(output.tangents);
                    geometry_data.texcoords = std::move(output.texcoords);
                    geometry_data.indexes = std::move(output.indexes);

                    for (std::size_t i = 0; i < geometry_data.submeshes.size(); ++i)
                    {
                        geometry_data.submeshes[i] = {
                            .clusters = std::move(output.submeshes[i].clusters),
                            .cluster_nodes = std::move(output.submeshes[i].cluster_nodes),
                        };
                    }

                    log::info(
                        "generate cluster: {} / {}",
                        total.fetch_add(1) + 1,
                        scene_data.geometries.size());
                }
            });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    return true;
}

bool mesh_loader_generate_mipmaps(mesh_loader::scene_data& scene_data)
{
    for (std::uint32_t i = 0; i < scene_data.textures.size(); ++i)
    {
        auto& src = scene_data.textures[i];
        texture_data dst = {};

        if (!texture_tool::generate_mipmaps(src, dst))
        {
            return false;
        }

        src = std::move(dst);

        log::info("generate mipmaps: {} / {}", i + 1, scene_data.textures.size());
    }

    return true;
}

bool mesh_loader_compress_textures(mesh_loader::scene_data& scene_data)
{
    for (std::uint32_t i = 0; i < scene_data.textures.size(); ++i)
    {
        auto& src = scene_data.textures[i];
        texture_data dst = {};

        switch (src.format)
        {
        case RHI_FORMAT_R8G8B8A8_UNORM: {
            dst.format = RHI_FORMAT_BC7_UNORM;
            break;
        }
        case RHI_FORMAT_R8G8B8A8_SRGB: {
            dst.format = RHI_FORMAT_BC7_SRGB;
            break;
        }
        default:
            return false;
        }

        if (!texture_tool::compress(src, dst))
        {
            return false;
        }

        src = std::move(dst);

        log::info("compress textures: {} / {}", i + 1, scene_data.textures.size());
    }

    return true;
}
} // namespace

bool mesh_loader::load(
    std::string_view path,
    scene_data& scene_data,
    bool generate_clusters,
    bool generate_mipmaps,
    bool compress_textures)
{
    namespace fs = std::filesystem;

    fs::path file(path);

    static constexpr std::string_view cache_dir = "assets/meshes";
    fs::create_directories(cache_dir);

    std::string cache_path = std::format(
        "{}/{}.mesh_{}{}{}",
        cache_dir,
        file.filename().string(),
        generate_clusters ? 1 : 0,
        generate_mipmaps ? 1 : 0,
        compress_textures ? 1 : 0);

    if (fs::exists(cache_path))
    {
        return load(cache_path, scene_data);
    }

    bool result = true;

    if (file.extension() == ".gltf" || file.extension() == ".glb")
    {
        gltf_loader loader;
        result = loader.load(path, scene_data);
    }
    else
    {
        assimp_loader loader;
        result = loader.load(path, scene_data);
    }

    if (result && generate_clusters)
    {
        result = mesh_loader_generate_clusters(scene_data);
    }

    if (result && generate_mipmaps)
    {
        result = mesh_loader_generate_mipmaps(scene_data);
    }

    if (result && compress_textures)
    {
        result = mesh_loader_compress_textures(scene_data);
    }

    if (result)
    {
        save(cache_path, scene_data);
    }

    return result;
}

bool mesh_loader::load(std::string_view path, scene_data& scene_data)
{
    std::ifstream fin(std::string(path), std::ios::binary);
    if (!fin.is_open())
    {
        return false;
    }

    std::uint32_t magic_number;
    read(fin, magic_number);

    if (magic_number != MAGIC_NUMBER)
    {
        return false;
    }

    std::uint32_t block_count;
    read(fin, block_count);

    for (std::uint32_t i = 0; i < block_count; ++i)
    {
        std::uint32_t block_type;
        read(fin, block_type);

        bool result = true;

        switch (block_type)
        {
        case BLOCK_NODE:
            result = read_node(fin, scene_data);
            break;
        case BLOCK_GEOMETRY:
            result = read_geometry(fin, scene_data);
            break;
        case BLOCK_MATERIAL:
            result = read_material(fin, scene_data);
            break;
        case BLOCK_TEXTURE:
            result = read_texture(fin, scene_data);
            break;
        default:
            return false;
        }

        if (!result)
        {
            return false;
        }
    }

    return true;
}

bool mesh_loader::save(std::string_view path, const scene_data& scene_data)
{
    std::ofstream fout(std::string(path), std::ios::binary);
    if (!fout.is_open())
    {
        return false;
    }

    write(fout, MAGIC_NUMBER);

    std::uint32_t block_count = 0;
    if (!scene_data.nodes.empty())
    {
        ++block_count;
    }

    if (!scene_data.geometries.empty())
    {
        ++block_count;
    }

    if (!scene_data.materials.empty())
    {
        ++block_count;
    }

    if (!scene_data.textures.empty())
    {
        ++block_count;
    }

    write(fout, block_count);

    bool result = true;

    if (result && !scene_data.nodes.empty())
    {
        write(fout, BLOCK_NODE);
        result = write_node(fout, scene_data);
    }

    if (result && !scene_data.geometries.empty())
    {
        write(fout, BLOCK_GEOMETRY);
        result = write_geometry(fout, scene_data);
    }

    if (result && !scene_data.materials.empty())
    {
        write(fout, BLOCK_MATERIAL);
        result = write_material(fout, scene_data);
    }

    if (result && !scene_data.textures.empty())
    {
        write(fout, BLOCK_TEXTURE);
        result = write_texture(fout, scene_data);
    }

    return result;
}
} // namespace violet