#ifndef MESH_HLSLI
#define MESH_HLSLI

#include "common.hlsli"

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

struct vertex
{
    float3 position;
    float3 normal;
    float4 tangent;
    float2 texcoord;

    float3 position_ws;
    float3 normal_ws;
    float3 tangent_ws;
    float3 bitangent_ws;

    float4 position_cs;
};

struct mesh
{
    uint instance_id;
    uint vertex_id;

    static mesh create(uint instance_id, uint vertex_id)
    {
        mesh result;
        result.instance_id = instance_id;
        result.vertex_id = vertex_id;
        return result;
    }

    vertex fetch_vertex()
    {
        StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];
        instance_data instance = instances[instance_id];

        StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
        mesh_data mesh = meshes[instance.mesh_index];

        ByteAddressBuffer vertex_buffer = ResourceDescriptorHeap[scene.vertex_buffer];

        vertex result;
        result.position = vertex_buffer.Load<float3>(mesh.position_address + vertex_id * sizeof(float3));
        result.position_ws = mul(mesh.model_matrix, float4(result.position, 1.0)).xyz;
        result.position_cs = mul(camera.view_projection, float4(result.position_ws, 1.0));

        if (mesh.normal_address != 0)
        {
            result.normal = vertex_buffer.Load<float3>(mesh.normal_address + vertex_id * sizeof(float3));
            result.normal_ws = mul((float3x3)mesh.model_matrix, result.normal);
        }

        if (mesh.tangent_address != 0)
        {
            result.tangent = vertex_buffer.Load<float4>(mesh.tangent_address + vertex_id * sizeof(float4));
            result.tangent_ws = mul((float3x3)mesh.model_matrix, result.tangent.xyz);
            result.bitangent_ws = normalize(cross(result.normal_ws, result.tangent_ws) * result.tangent.w);
        }

        if (mesh.texcoord_address != 0)
        {
            result.texcoord = vertex_buffer.Load<float2>(mesh.texcoord_address + vertex_id * sizeof(float2));
        }

        return result;
    }

    template <typename T>
    T fetch_custom_attribute(uint index)
    {
        StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];
        instance_data instance = instances[instance_id];

        StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
        mesh_data mesh = meshes[instance.mesh_index];

        ByteAddressBuffer vertex_buffer = ResourceDescriptorHeap[scene.vertex_buffer];

        return vertex_buffer.Load<T>(mesh.custom_addresses[index] + vertex_id * sizeof(T));
    }

    uint get_material_address()
    {
        StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];
        instance_data instance = instances[instance_id];
        return instance.material_address;
    }

    float4x4 get_model_matrix()
    {
        StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];
        instance_data instance = instances[instance_id];

        StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
        mesh_data mesh = meshes[instance.mesh_index];

        return mesh.model_matrix;
    }
};

#endif