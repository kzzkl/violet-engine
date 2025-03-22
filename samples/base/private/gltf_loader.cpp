#include "gltf_loader.hpp"
#include "graphics/materials/physical_material.hpp"
#include "graphics/tools/geometry_tool.hpp"
#include <filesystem>
#include <iostream>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"

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
    const tinygltf::Accessor& accessor =
        model.accessors[primitive.attributes.find(name.data())->second];

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
    const tinygltf::Accessor& accessor = model.accessors[primitive.indices];

    const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];

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

gltf_loader::gltf_loader(std::string_view path)
    : m_path(path)
{
}

gltf_loader::~gltf_loader() = default;

std::optional<mesh_loader::scene_data> gltf_loader::load()
{
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;

    std::string error;
    std::string warn;

    bool result = false;
    if (m_path.ends_with(".gltf"))
    {
        result = loader.LoadASCIIFromFile(&model, &error, &warn, m_path);
    }
    else if (m_path.ends_with(".glb"))
    {
        result = loader.LoadBinaryFromFile(&model, &error, &warn, m_path);
    }

    if (!warn.empty())
    {
        std::cout << "WARN: " << warn << std::endl;
    }

    if (!error.empty())
    {
        std::cout << "ERROR: " << error << std::endl;
    }

    if (!result)
    {
        return std::nullopt;
    }

    std::filesystem::path model_path(m_path);
    std::filesystem::path dir_path = model_path.parent_path();

    mesh_loader::scene_data scene_data;

    // Load textures
    auto get_format = [](int component, int bits, int pixel_type) -> rhi_format
    {
        if (component == 4 && bits == 8 && pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
        {
            return RHI_FORMAT_R8G8B8A8_UNORM;
        }

        return RHI_FORMAT_UNDEFINED;
    };

    auto get_texture = [&](int index, bool srgb = false) -> texture_2d*
    {
        int image_index = model.textures[index].source;

        if (scene_data.textures.size() <= image_index)
        {
            scene_data.textures.resize(image_index + 1);
        }

        if (scene_data.textures[image_index] != nullptr)
        {
            return scene_data.textures[image_index].get();
        }

        texture_options options = srgb ? TEXTURE_OPTION_SRGB : TEXTURE_OPTION_NONE;

        auto& image = model.images[image_index];
        if (!image.uri.empty())
        {
            std::filesystem::path texture_path = dir_path / image.uri;
            scene_data.textures[image_index] =
                std::make_unique<texture_2d>(texture_path.string(), options);
        }
        else
        {
            texture_data data = {
                .format = get_format(image.component, image.bits, image.pixel_type),
            };
            data.mipmaps.resize(1);
            data.mipmaps[0].extent.height = image.height;
            data.mipmaps[0].extent.width = image.width;
            data.mipmaps[0].pixels.resize(image.image.size());
            std::memcpy(data.mipmaps[0].pixels.data(), image.image.data(), image.image.size());
            scene_data.textures[image_index] = std::make_unique<texture_2d>(data, options);
        }

        return scene_data.textures[image_index].get();
    };

    // Load materials
    for (auto& material : model.materials)
    {
        auto pbr_material = std::make_unique<physical_material>();
        vec3f albedo = {
            .r = static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[0]),
            .g = static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[1]),
            .b = static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[2]),
        };
        pbr_material->set_albedo(albedo);
        if (material.pbrMetallicRoughness.baseColorTexture.index != -1)
        {
            pbr_material->set_albedo(
                get_texture(material.pbrMetallicRoughness.baseColorTexture.index, true));
        }

        pbr_material->set_roughness(
            static_cast<float>(material.pbrMetallicRoughness.roughnessFactor));
        pbr_material->set_metallic(
            static_cast<float>(material.pbrMetallicRoughness.metallicFactor));
        if (material.pbrMetallicRoughness.metallicRoughnessTexture.index != -1)
        {
            pbr_material->set_roughness_metallic(
                get_texture(material.pbrMetallicRoughness.metallicRoughnessTexture.index));
        }

        pbr_material->set_emissive({
            static_cast<float>(material.emissiveFactor[0]),
            static_cast<float>(material.emissiveFactor[1]),
            static_cast<float>(material.emissiveFactor[2]),
        });
        if (material.emissiveTexture.index != -1)
        {
            pbr_material->set_emissive(get_texture(material.emissiveTexture.index, true));
        }

        if (material.normalTexture.index != -1)
        {
            pbr_material->set_normal(get_texture(material.normalTexture.index));
        }

        scene_data.materials.push_back(std::move(pbr_material));
    }

    // Load meshes
    for (auto& mesh : model.meshes)
    {
        std::vector<vec3f> positions;
        std::vector<vec3f> normals;
        std::vector<vec4f> tangents;
        std::vector<vec2f> texcoords;
        std::vector<std::uint32_t> indexes;

        mesh_data mesh_data = {
            .geometry = static_cast<std::uint32_t>(scene_data.geometries.size()),
        };

        for (auto& primitive : mesh.primitives)
        {
            submesh_data submesh_data = {
                .vertex_offset = static_cast<std::uint32_t>(positions.size()),
                .index_offset = static_cast<std::uint32_t>(indexes.size()),
                .material = primitive.material,
            };

            // Load indexes
            submesh_data.index_count = load_indexes(model, primitive, indexes);

            // Load attributes
            std::uint32_t vertex_count = load_attribute(model, primitive, "POSITION", positions);

            if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
            {
                load_attribute(model, primitive, "TEXCOORD_0", texcoords);
            }
            else
            {
                texcoords.resize(positions.size());
            }

            if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
            {
                load_attribute(model, primitive, "NORMAL", normals);
            }
            else
            {
                normals.resize(positions.size());
            }

            if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
            {
                load_attribute(model, primitive, "TANGENT", tangents);
            }
            else
            {
                tangents = geometry_tool::generate_tangents(
                    std::span(
                        positions.begin() + submesh_data.vertex_offset,
                        positions.begin() + submesh_data.vertex_offset + vertex_count),
                    std::span(
                        normals.begin() + submesh_data.vertex_offset,
                        normals.begin() + submesh_data.vertex_offset + vertex_count),
                    std::span(
                        texcoords.begin() + submesh_data.vertex_offset,
                        texcoords.begin() + submesh_data.vertex_offset + vertex_count),
                    std::span(
                        indexes.begin() + submesh_data.index_offset,
                        indexes.begin() + submesh_data.index_offset + submesh_data.index_count));
            }

            mesh_data.submeshes.push_back(submesh_data);

            for (std::uint32_t i = 0; i < submesh_data.index_count; ++i)
            {
                box::expand(
                    mesh_data.aabb,
                    positions[indexes[submesh_data.index_offset + i] + submesh_data.vertex_offset]);
            }
        }

        if (tangents.empty())
        {
            tangents.resize(positions.size());
            if (!normals.empty() && !texcoords.empty())
            {
                auto temp =
                    geometry_tool::generate_tangents(positions, normals, texcoords, indexes);
                std::transform(
                    temp.begin(),
                    temp.end(),
                    tangents.begin(),
                    [](const vec3f& t) -> vec4f
                    {
                        return {t.x, t.y, t.z, 1.0f};
                    });
            }
        }

        if (normals.empty())
        {
            normals.resize(positions.size());
        }

        if (texcoords.empty())
        {
            texcoords.resize(positions.size());
        }

        scene_data.meshes.push_back(mesh_data);

        auto mesh_geometry = std::make_unique<geometry>();
        mesh_geometry->set_position(positions);
        mesh_geometry->set_normal(normals);
        mesh_geometry->set_tangent(tangents);
        mesh_geometry->set_texcoord(texcoords);
        mesh_geometry->set_indexes(indexes);

        scene_data.geometries.push_back(std::move(mesh_geometry));
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

    return scene_data;
}
} // namespace violet