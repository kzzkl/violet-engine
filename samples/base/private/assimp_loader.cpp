#include "sample/assimp_loader.hpp"
#include "math/vector.hpp"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <filesystem>

namespace violet
{
namespace
{
void process_mesh(aiMesh* mesh, mesh_loader::scene_data& scene_data)
{
    mesh_loader::geometry_data geometry_data = {};

    for (std::uint32_t i = 0; i < mesh->mNumVertices; ++i)
    {
        vec3f position;
        position.x = mesh->mVertices[i].x;
        position.y = mesh->mVertices[i].y;
        position.z = mesh->mVertices[i].z;
        geometry_data.positions.push_back(position);

        vec3f normal;
        normal.x = mesh->mNormals[i].x;
        normal.y = mesh->mNormals[i].y;
        normal.z = mesh->mNormals[i].z;
        geometry_data.normals.push_back(normal);

        if (mesh->mTangents && mesh->mBitangents)
        {
            vec4f tangent;
            tangent.x = mesh->mTangents[i].x;
            tangent.y = mesh->mTangents[i].y;
            tangent.z = mesh->mTangents[i].z;

            vec3f bitangent;
            bitangent.x = mesh->mBitangents[i].x;
            bitangent.y = mesh->mBitangents[i].y;
            bitangent.z = mesh->mBitangents[i].z;

            if (vector::dot(vector::cross(normal, vec3f(tangent)), bitangent) < 0.0f)
            {
                tangent.w = -1.0f;
            }
            else
            {
                tangent.w = 1.0f;
            }
            geometry_data.tangents.push_back(tangent);
        }

        if (mesh->mTextureCoords[0])
        {
            vec2f uv;
            uv.x = mesh->mTextureCoords[0][i].x;
            uv.y = mesh->mTextureCoords[0][i].y;
            geometry_data.texcoords.push_back(uv);
        }
    }

    for (std::uint32_t i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace face = mesh->mFaces[i];
        for (std::uint32_t j = 0; j < face.mNumIndices; ++j)
        {
            geometry_data.indexes.push_back(face.mIndices[j]);
        }
    }

    geometry_data.submeshes.emplace_back(
        mesh_loader::submesh_data{
            .vertex_offset = 0,
            .index_offset = 0,
            .index_count = static_cast<std::uint32_t>(geometry_data.indexes.size()),
        });

    scene_data.geometries.emplace_back(std::move(geometry_data));
}

void process_node(aiNode* node, std::int32_t parnet, mesh_loader::scene_data& scene_data)
{
    mesh_loader::node_data node_data = {
        .name = node->mName.C_Str(),
        .parent = parnet,
    };

    aiVector3f scale;
    aiQuaternion rotation;
    aiVector3f position;
    node->mTransformation.Decompose(scale, rotation, position);
    node_data.position = {.x = position.x, .y = position.y, .z = position.z};
    node_data.rotation = {.x = rotation.x, .y = rotation.y, .z = rotation.z, .w = rotation.w};
    node_data.scale = {.x = scale.x, .y = scale.y, .z = scale.z};

    assert(node->mNumMeshes <= 1);

    for (std::uint32_t i = 0; i < node->mNumMeshes; i++)
    {
        node_data.mesh = static_cast<std::int32_t>(scene_data.meshes.size());

        mesh_loader::mesh_data mesh_data = {
            .geometry = node->mMeshes[i],
        };
        mesh_data.submeshes.push_back(0);
        mesh_data.materials.push_back(-1);
        scene_data.meshes.push_back(mesh_data);
    }

    auto node_index = static_cast<std::int32_t>(scene_data.nodes.size());
    scene_data.nodes.push_back(node_data);

    for (std::uint32_t i = 0; i < node->mNumChildren; i++)
    {
        process_node(node->mChildren[i], node_index, scene_data);
    }
}
} // namespace

std::optional<mesh_loader::scene_data> assimp_loader::load(std::string_view path)
{
    std::filesystem::path model_path(path);

    Assimp::Importer importer;

    std::uint32_t flags = 0;
    flags |= aiProcess_Triangulate;
    flags |= aiProcess_GenNormals;
    flags |= aiProcess_CalcTangentSpace;
    flags |= aiProcess_JoinIdenticalVertices;
    flags |= aiProcess_ImproveCacheLocality;
    flags |= aiProcess_ConvertToLeftHanded;

    const aiScene* scene = importer.ReadFile(model_path.string(), flags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        return std::nullopt;
    }

    scene_data scene_data;

    for (std::uint32_t i = 0; i < scene->mNumMeshes; ++i)
    {
        process_mesh(scene->mMeshes[i], scene_data);
    }

    process_node(scene->mRootNode, -1, scene_data);

    return scene_data;
}
} // namespace violet