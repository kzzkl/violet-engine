#include "fbx_loader.hpp"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

namespace violet::sample
{
std::unique_ptr<geometry> fbx_loader::load(std::string_view path, float3* center)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        path.data(),
        aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_MakeLeftHanded |
            aiProcess_FlipUVs);
    if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        scene->mRootNode == nullptr || scene->mNumMeshes == 0)
    {
        return nullptr;
    }

    std::vector<float3> positions;
    std::vector<float3> normals;
    std::vector<float3> tangents;
    std::vector<float2> uvs;
    std::vector<std::uint32_t> indices;

    vector4f bounding_box_min = vector::replicate(std::numeric_limits<float>::max());
    vector4f bounding_box_max = vector::replicate(std::numeric_limits<float>::min());

    for (std::size_t i = 0; i < scene->mNumMeshes; ++i)
    {
        const aiMesh* mesh = scene->mMeshes[i];
        if (mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
            continue;

        for (std::size_t j = 0; j < mesh->mNumFaces; ++j)
        {
            aiFace& face = mesh->mFaces[j];
            if (face.mNumIndices != 3)
                continue;

            indices.push_back(face.mIndices[0] + positions.size());
            indices.push_back(face.mIndices[1] + positions.size());
            indices.push_back(face.mIndices[2] + positions.size());
        }

        positions.reserve(positions.size() + mesh->mNumVertices);
        for (std::size_t j = 0; j < mesh->mNumVertices; ++j)
        {
            aiVector3f& vertex = mesh->mVertices[j];
            positions.push_back(float3{vertex.x, vertex.y, vertex.z});

            vector4f position = math::load(positions.back());
            bounding_box_min = vector::min(bounding_box_min, position);
            bounding_box_max = vector::max(bounding_box_min, position);
        }

        if (mesh->mNormals != nullptr)
        {
            normals.reserve(normals.size() + mesh->mNumVertices);
            for (std::size_t j = 0; j < mesh->mNumVertices; ++j)
            {
                aiVector3f& normal = mesh->mNormals[j];
                normals.push_back(float3{normal.x, normal.y, normal.z});
            }
        }

        if (mesh->mTangents != nullptr)
        {
            tangents.reserve(tangents.size() + mesh->mNumVertices);
            for (std::size_t j = 0; j < mesh->mNumVertices; ++j)
            {
                aiVector3f& tangent = mesh->mTangents[j];
                tangents.push_back(float3{tangent.x, tangent.y, tangent.z});
            }
        }

        if (mesh->mTextureCoords[0] != nullptr)
        {
            uvs.reserve(uvs.size() + mesh->mNumVertices);
            for (std::size_t j = 0; j < mesh->mNumVertices; ++j)
            {
                aiVector3f& uv = mesh->mTextureCoords[0][j];
                uvs.push_back(float2{uv.x, uv.y});
            }
        }
    }

    vector4f bounding_box_center =
        vector::mul(vector::add(bounding_box_min, bounding_box_max), 0.5f);
    if (center != nullptr)
        math::store(bounding_box_center, *center);

    importer.FreeScene();

    auto result = std::make_unique<geometry>();
    result->add_attribute("position", positions);

    if (!normals.empty())
        result->add_attribute("normal", normals);
    if (!tangents.empty())
        result->add_attribute("tangent", tangents);
    if (!uvs.empty())
        result->add_attribute("uv", uvs);

    result->set_indices(indices);

    return result;
}
} // namespace violet::sample