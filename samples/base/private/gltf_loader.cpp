#include "gltf_loader.hpp"
#include "tools/geometry_tool.hpp"
#include "tools/texture_tool.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>

namespace violet
{
namespace
{
template <typename T>
std::uint32_t load_attribute(
    const tinygltf::Model& model,
    const tinygltf::Primitive& primitive,
    std::string_view name,
    std::vector<T>& attribute)
{
    const auto& accessor = model.accessors[primitive.attributes.at(std::string(name))];

    assert(
        static_cast<std::size_t>(
            tinygltf::GetComponentSizeInBytes(accessor.componentType) *
            tinygltf::GetNumComponentsInType(accessor.type)) == sizeof(T));

    const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];

    const auto* buffer = reinterpret_cast<const float*>(
        &(model.buffers[buffer_view.buffer].data[accessor.byteOffset + buffer_view.byteOffset]));

    const std::size_t offset = attribute.size();
    attribute.resize(attribute.size() + accessor.count);
    std::memcpy(attribute.data() + offset, buffer, sizeof(T) * accessor.count);

    return static_cast<std::uint32_t>(accessor.count);
};

template <typename T>
std::uint32_t load_indexes(
    const tinygltf::Model& model,
    const tinygltf::Primitive& primitive,
    std::vector<T>& indexes)
{
    const auto& accessor = model.accessors[primitive.indices];
    const auto& buffer_view = model.bufferViews[accessor.bufferView];

    const void* buffer =
        &(model.buffers[buffer_view.buffer].data[accessor.byteOffset + buffer_view.byteOffset]);

    switch (accessor.componentType)
    {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
        std::vector<std::uint16_t> indexes_u16(
            static_cast<const std::uint16_t*>(buffer),
            static_cast<const std::uint16_t*>(buffer) + accessor.count);
        for (std::uint16_t index : indexes_u16)
        {
            indexes.push_back(index);
        }
        break;
    }
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
        std::vector<std::uint32_t> indexes_u32(
            static_cast<const std::uint32_t*>(buffer),
            static_cast<const std::uint32_t*>(buffer) + accessor.count);
        for (std::uint32_t index : indexes_u32)
        {
            indexes.push_back(index);
        }
        break;
    }
    default:
        throw std::runtime_error("Invalid index type.");
        break;
    }

    return static_cast<std::uint32_t>(accessor.count);
}
} // namespace

bool gltf_loader::load(std::string_view path, mesh_loader::scene_data& scene_data)
{
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;

    std::string error;
    std::string warn;

    bool result = false;
    if (path.ends_with(".gltf"))
    {
        result = loader.LoadASCIIFromFile(&model, &error, &warn, std::string(path));
    }
    else if (path.ends_with(".glb"))
    {
        result = loader.LoadBinaryFromFile(&model, &error, &warn, std::string(path));
    }

    if (!warn.empty())
    {
        std::cout << "WARN: " << warn << '\n';
    }

    if (!error.empty())
    {
        std::cout << "ERROR: " << error << '\n';
    }

    if (!result)
    {
        return false;
    }

    std::filesystem::path model_path(path);
    std::filesystem::path dir_path = model_path.parent_path();

    std::unordered_map<int, int> image_to_texture;

    // Load textures
    auto get_texture = [&](int index, rhi_format format) -> std::int32_t
    {
        int image_index = model.textures[index].source;
        if (image_to_texture.contains(image_index))
        {
            return image_to_texture[image_index];
        }

        int texture_index = static_cast<int>(scene_data.textures.size());
        image_to_texture[image_index] = texture_index;

        auto& texture = scene_data.textures.emplace_back();

        auto& image = model.images[image_index];
        if (!image.uri.empty())
        {
            std::filesystem::path texture_path = dir_path / image.uri;
            texture_tool::load(texture_path.string(), texture);

            if (format == RHI_FORMAT_R8G8B8A8_SRGB)
            {
                texture.format = RHI_FORMAT_R8G8B8A8_SRGB;
            }
        }
        else
        {
            texture = {
                .layer_count = 1,
                .level_count = 1,
            };

            if (format == RHI_FORMAT_R8G8B8A8_SRGB)
            {
                texture.format = RHI_FORMAT_R8G8B8A8_SRGB;
            }
            else if (
                image.component == 4 && image.bits == 8 &&
                image.pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
            {
                texture.format = RHI_FORMAT_R8G8B8A8_UNORM;
            }

            texture.extent.height = image.height;
            texture.extent.width = image.width;
            texture.pixels.resize(image.image.size());
            std::memcpy(texture.pixels.data(), image.image.data(), image.image.size());
        }

        return texture_index;
    };

    // Load materials
    for (auto& material : model.materials)
    {
        mesh_loader::material_data material_data = {
            .cull_mode = material.doubleSided ? RHI_CULL_MODE_NONE : RHI_CULL_MODE_BACK,
            .opacity_cutoff =
                material.alphaMode == "MASK" ? static_cast<float>(material.alphaCutoff) : 0.0f,
        };

        material_data.albedo = {
            .x = static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[0]),
            .y = static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[1]),
            .z = static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[2]),
        };

        if (material.pbrMetallicRoughness.baseColorTexture.index != -1)
        {
            material_data.albedo_texture = get_texture(
                material.pbrMetallicRoughness.baseColorTexture.index,
                RHI_FORMAT_R8G8B8A8_SRGB);
        }

        material_data.roughness = static_cast<float>(material.pbrMetallicRoughness.roughnessFactor);
        material_data.metallic = static_cast<float>(material.pbrMetallicRoughness.metallicFactor);
        if (material.pbrMetallicRoughness.metallicRoughnessTexture.index != -1)
        {
            material_data.roughness_metallic_texture = get_texture(
                material.pbrMetallicRoughness.metallicRoughnessTexture.index,
                RHI_FORMAT_R8G8B8A8_UNORM);
        }

        material_data.emissive = {
            .x = static_cast<float>(material.emissiveFactor[0]),
            .y = static_cast<float>(material.emissiveFactor[1]),
            .z = static_cast<float>(material.emissiveFactor[2]),
        };
        if (material.emissiveTexture.index != -1)
        {
            material_data.emissive_texture =
                get_texture(material.emissiveTexture.index, RHI_FORMAT_R8G8B8A8_SRGB);
        }

        if (material.normalTexture.index != -1)
        {
            material_data.normal_texture =
                get_texture(material.normalTexture.index, RHI_FORMAT_R8G8B8A8_UNORM);
        }

        scene_data.materials.push_back(material_data);
    }

    // Load meshes
    for (auto& mesh : model.meshes)
    {
        mesh_loader::geometry_data geometry_data = {};

        for (auto& primitive : mesh.primitives)
        {
            auto vertex_offset = static_cast<std::uint32_t>(geometry_data.positions.size());
            auto index_offset = static_cast<std::uint32_t>(geometry_data.indexes.size());

            std::int32_t material = primitive.material;

            // Load indexes
            std::uint32_t index_count = load_indexes(model, primitive, geometry_data.indexes);

            // Load attributes
            std::uint32_t vertex_count =
                load_attribute(model, primitive, "POSITION", geometry_data.positions);

            if (primitive.attributes.contains("TEXCOORD_0"))
            {
                load_attribute(model, primitive, "TEXCOORD_0", geometry_data.texcoords);
            }
            else
            {
                geometry_data.texcoords.resize(geometry_data.positions.size());
            }

            if (primitive.attributes.contains("NORMAL"))
            {
                load_attribute(model, primitive, "NORMAL", geometry_data.normals);
            }
            else
            {
                geometry_data.normals.resize(
                    geometry_data.positions.size(),
                    vec3f(0.0f, 1.0f, 0.0f));
            }

            if (primitive.attributes.contains("TANGENT"))
            {
                load_attribute(model, primitive, "TANGENT", geometry_data.tangents);
            }
            else
            {
                auto tangents = geometry_tool::generate_tangents(
                    std::span(
                        geometry_data.positions.begin() + vertex_offset,
                        geometry_data.positions.begin() + vertex_offset + vertex_count),
                    std::span(
                        geometry_data.normals.begin() + vertex_offset,
                        geometry_data.normals.begin() + vertex_offset + vertex_count),
                    std::span(
                        geometry_data.texcoords.begin() + vertex_offset,
                        geometry_data.texcoords.begin() + vertex_offset + vertex_count),
                    std::span(
                        geometry_data.indexes.begin() + index_offset,
                        geometry_data.indexes.begin() + index_offset + index_count));

                geometry_data.tangents.insert(
                    geometry_data.tangents.end(),
                    tangents.begin(),
                    tangents.end());
            }

            geometry_data.submeshes.push_back({
                .vertex_offset = vertex_offset,
                .index_offset = index_offset,
                .index_count = index_count,
            });
        }

        if (geometry_data.tangents.empty())
        {
            geometry_data.tangents.resize(geometry_data.positions.size());
            if (!geometry_data.normals.empty() && !geometry_data.texcoords.empty())
            {
                auto temp = geometry_tool::generate_tangents(
                    geometry_data.positions,
                    geometry_data.normals,
                    geometry_data.texcoords,
                    geometry_data.indexes);
                std::ranges::transform(
                    temp,
                    geometry_data.tangents.begin(),
                    [](const vec3f& t) -> vec4f
                    {
                        return {t.x, t.y, t.z, 1.0f};
                    });
            }
        }

        if (geometry_data.normals.empty())
        {
            geometry_data.normals.resize(geometry_data.positions.size());
        }

        if (geometry_data.texcoords.empty())
        {
            geometry_data.texcoords.resize(geometry_data.positions.size());
        }

        // Convert to left-handed coordinate system.
        for (vec3f& position : geometry_data.positions)
        {
            position.x = -position.x;
        }

        for (vec3f& normal : geometry_data.normals)
        {
            normal.x = -normal.x;
        }

        for (vec4f& tangent : geometry_data.tangents)
        {
            tangent.x = -tangent.x;
            tangent.w = -tangent.w;
        }

        mesh_loader::mesh_data mesh_data = {
            .geometry = static_cast<std::uint32_t>(scene_data.geometries.size()),
        };

        for (std::uint32_t i = 0; i < mesh.primitives.size(); ++i)
        {
            mesh_data.submeshes.push_back(i);
            mesh_data.materials.push_back(mesh.primitives[i].material);
        }

        scene_data.meshes.push_back(mesh_data);
        scene_data.geometries.push_back(std::move(geometry_data));
    }

    // Load nodes
    scene_data.nodes.resize(model.nodes.size());
    for (std::size_t i = 0; i < model.nodes.size(); ++i)
    {
        for (int child : model.nodes[i].children)
        {
            scene_data.nodes[child].parent = static_cast<std::int32_t>(i);
        }

        scene_data.nodes[i].name = model.nodes[i].name;
        scene_data.nodes[i].mesh = model.nodes[i].mesh;

        if (!model.nodes[i].translation.empty())
        {
            scene_data.nodes[i].position.x = static_cast<float>(model.nodes[i].translation[0]);
            scene_data.nodes[i].position.y = static_cast<float>(model.nodes[i].translation[1]);
            scene_data.nodes[i].position.z = static_cast<float>(model.nodes[i].translation[2]);
        }

        if (!model.nodes[i].rotation.empty())
        {
            scene_data.nodes[i].rotation.x = static_cast<float>(model.nodes[i].rotation[0]);
            scene_data.nodes[i].rotation.y = static_cast<float>(model.nodes[i].rotation[1]);
            scene_data.nodes[i].rotation.z = static_cast<float>(model.nodes[i].rotation[2]);
            scene_data.nodes[i].rotation.w = static_cast<float>(model.nodes[i].rotation[3]);
        }

        if (!model.nodes[i].scale.empty())
        {
            scene_data.nodes[i].scale.x = static_cast<float>(model.nodes[i].scale[0]);
            scene_data.nodes[i].scale.y = static_cast<float>(model.nodes[i].scale[1]);
            scene_data.nodes[i].scale.z = static_cast<float>(model.nodes[i].scale[2]);
        }
    }

    return true;
}
} // namespace violet