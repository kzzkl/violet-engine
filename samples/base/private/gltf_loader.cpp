#include "gltf_loader.hpp"
#include "graphics/materials/physical_material.hpp"
#include "mikktspace.h"
#include <filesystem>
#include <iostream>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"

namespace violet
{
namespace
{
template <typename T>
void load_attribute(
    const tinygltf::Model& model,
    const tinygltf::Primitive& primitive,
    std::string_view name,
    std::vector<T>& attribute)
{
    const tinygltf::Accessor& accessor =
        model.accessors[primitive.attributes.find(name.data())->second];
    const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];

    const auto* buffer = reinterpret_cast<const float*>(
        &(model.buffers[buffer_view.buffer].data[accessor.byteOffset + buffer_view.byteOffset]));

    const std::size_t offset = attribute.size();
    attribute.resize(attribute.size() + accessor.count);
    std::memcpy(attribute.data() + offset, buffer, sizeof(T) * accessor.count);
};

template <typename T>
void load_indexes(
    const tinygltf::Model& model,
    const tinygltf::Primitive& primitive,
    std::vector<T>& indexes)
{
    const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
    const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];

    const auto* buffer = reinterpret_cast<const float*>(
        &(model.buffers[buffer_view.buffer].data[accessor.byteOffset + buffer_view.byteOffset]));

    const std::size_t offset = indexes.size();
    indexes.resize(indexes.size() + accessor.count);
    std::memcpy(indexes.data() + offset, buffer, sizeof(T) * accessor.count);
}

bool generate_tangents(
    const std::vector<vec3f>& positions,
    const std::vector<vec3f>& normals,
    const std::vector<vec2f>& texcoords,
    const std::vector<std::uint32_t>& indexes,
    std::vector<vec3f>& tangents)
{
    tangents.resize(positions.size());

    struct tangent_context : public SMikkTSpaceContext
    {
        const std::vector<vec3f>& positions;
        const std::vector<vec3f>& normals;
        const std::vector<vec2f>& texcoords;
        const std::vector<std::uint32_t>& indexes;

        std::vector<vec3f>& tangents;
    };

    SMikkTSpaceInterface interface = {};
    interface.m_getNumFaces = [](const SMikkTSpaceContext* context) -> int
    {
        const auto* ctx = static_cast<const tangent_context*>(context);
        return static_cast<int>(ctx->indexes.size() / 3);
    };
    interface.m_getNumVerticesOfFace = [](const SMikkTSpaceContext* context, const int face) -> int
    {
        return 3;
    };
    interface.m_getPosition =
        [](const SMikkTSpaceContext* context, float* out, const int face, const int vert)
    {
        const auto* ctx = static_cast<const tangent_context*>(context);
        const vec3f& position = ctx->positions[ctx->indexes[face * 3 + vert]];
        out[0] = position.x;
        out[1] = position.y;
        out[2] = position.z;
    };
    interface.m_getNormal =
        [](const SMikkTSpaceContext* context, float* out, const int face, const int vert)
    {
        const auto* ctx = static_cast<const tangent_context*>(context);
        const vec3f& normal = ctx->normals[ctx->indexes[face * 3 + vert]];
        out[0] = normal.x;
        out[1] = normal.y;
        out[2] = normal.z;
    };
    interface.m_getTexCoord =
        [](const SMikkTSpaceContext* context, float* out, const int face, const int vert)
    {
        const auto* ctx = static_cast<const tangent_context*>(context);
        const vec2f& texcoord = ctx->texcoords[ctx->indexes[face * 3 + vert]];
        out[0] = texcoord.x;
        out[1] = texcoord.y;
    };
    interface.m_setTSpaceBasic = [](const SMikkTSpaceContext* context,
                                    const float* in,
                                    const float sign,
                                    const int face,
                                    const int vert)
    {
        const auto* ctx = static_cast<const tangent_context*>(context);
        vec3f& tangent = ctx->tangents[ctx->indexes[face * 3 + vert]];
        tangent.x = in[0];
        tangent.y = in[1];
        tangent.z = in[2];
    };

    tangent_context context = {
        .positions = positions,
        .normals = normals,
        .texcoords = texcoords,
        .indexes = indexes,
        .tangents = tangents,
    };
    context.m_pInterface = &interface;

    return genTangSpaceDefault(&context);
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

    bool result = loader.LoadASCIIFromFile(&model, &error, &warn, m_path);

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
    std::vector<std::vector<std::uint8_t>> buffers;

    // Load textures
    auto get_texture = [&](int index, bool srgb = false) -> texture_2d*
    {
        if (scene_data.textures.size() <= index)
        {
            scene_data.textures.resize(index + 1);
        }

        if (scene_data.textures[index] != nullptr)
        {
            return scene_data.textures[index].get();
        }

        if (model.images[index].mimeType.empty())
        {
            std::filesystem::path texture_path = dir_path / model.images[index].uri;
            scene_data.textures[index] = std::make_unique<texture_2d>(
                texture_path.string(),
                srgb ? TEXTURE_OPTION_SRGB : TEXTURE_OPTION_NONE);
        }
        else
        {
            return nullptr;
        }

        return scene_data.textures[index].get();
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
        pbr_material->set_albedo(
            get_texture(material.pbrMetallicRoughness.baseColorTexture.index, true));

        pbr_material->set_roughness(
            static_cast<float>(material.pbrMetallicRoughness.roughnessFactor));
        pbr_material->set_metallic(
            static_cast<float>(material.pbrMetallicRoughness.metallicFactor));
        pbr_material->set_roughness_metallic(
            get_texture(material.pbrMetallicRoughness.metallicRoughnessTexture.index));

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

    // Load buffers
    for (auto& buffer : model.buffers)
    {
        if (!buffer.uri.empty())
        {
            std::filesystem::path buffer_path = dir_path / buffer.uri;

            std::ifstream fin(buffer_path, std::ios::binary);
            if (!fin.is_open())
            {
                return std::nullopt;
            }

            std::vector<std::uint8_t> data(std::filesystem::file_size(buffer_path));

            // Read the whole file into the buffer.
            fin.read(
                reinterpret_cast<char*>(data.data()),
                static_cast<std::streamsize>(data.size()));

            fin.close();

            buffers.push_back(std::move(data));
        }
        else
        {
            return std::nullopt;
        }
    }

    // Load meshes
    for (auto& mesh : model.meshes)
    {
        mesh_data mesh_data;
        mesh_data.geometry = static_cast<std::uint32_t>(scene_data.geometries.size());

        std::vector<vec3f> positions;
        std::vector<vec3f> normals;
        std::vector<vec3f> tangents;
        std::vector<vec2f> texcoords;
        std::vector<std::uint32_t> indexes;

        for (auto& primitive : mesh.primitives)
        {
            submesh_data submesh_data = {
                .vertex_offset = static_cast<std::uint32_t>(positions.size()),
                .index_offset = static_cast<std::uint32_t>(indexes.size()),
                .material = static_cast<std::uint32_t>(primitive.material),
            };

            // Load attributes
            load_attribute(model, primitive, "POSITION", positions);

            if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
            {
                load_attribute(model, primitive, "NORMAL", normals);
            }

            if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
            {
                load_attribute(model, primitive, "TANGENT", tangents);
            }

            if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
            {
                load_attribute(model, primitive, "TEXCOORD_0", texcoords);
            }

            // Load indexes
            const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
            switch (accessor.componentType)
            {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                std::vector<std::uint16_t> indexes_u16;
                load_indexes(model, primitive, indexes_u16);
                for (auto& index : indexes_u16)
                {
                    indexes.push_back(index + submesh_data.vertex_offset);
                }
                break;
            }
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                std::vector<std::uint32_t> indexes_u32;
                load_indexes(model, primitive, indexes_u32);
                for (auto& index : indexes_u32)
                {
                    indexes.push_back(index + submesh_data.vertex_offset);
                }
                break;
            }
            default:
                return std::nullopt;
            }

            submesh_data.index_count = accessor.count;

            mesh_data.submeshes.push_back(submesh_data);
            scene_data.meshes.push_back(mesh_data);
        }

        if (tangents.empty())
        {
            if (!generate_tangents(positions, normals, texcoords, indexes, tangents))
            {
                return std::nullopt;
            }
        }

        auto mesh_geometry = std::make_unique<geometry>();
        mesh_geometry->add_attribute("position", positions);
        mesh_geometry->add_attribute("normal", normals);
        mesh_geometry->add_attribute("tangent", tangents);
        mesh_geometry->add_attribute("texcoord", texcoords);
        mesh_geometry->set_indexes(indexes);
        mesh_geometry->set_vertex_count(positions.size());
        mesh_geometry->set_index_count(indexes.size());

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