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

        StructuredBuffer<geometry_data> geometries = ResourceDescriptorHeap[scene.geometry_buffer];
        geometry_data geometry = geometries[instance.geometry_index];

        StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
        mesh_data mesh = meshes[instance.mesh_index];

        ByteAddressBuffer vertex_buffer = ResourceDescriptorHeap[scene.vertex_buffer];

        vertex result;
        result.position = vertex_buffer.Load<float3>(geometry.position_address + vertex_id * sizeof(float3));
        result.position_ws = mul(mesh.matrix_m, float4(result.position, 1.0)).xyz;
        result.position_cs = mul(camera.matrix_vp, float4(result.position_ws, 1.0));

        if (geometry.normal_address != 0)
        {
            result.normal = vertex_buffer.Load<float3>(geometry.normal_address + vertex_id * sizeof(float3));
            result.normal_ws = mul((float3x3)mesh.matrix_m, result.normal);
        }

        if (geometry.tangent_address != 0)
        {
            result.tangent = vertex_buffer.Load<float4>(geometry.tangent_address + vertex_id * sizeof(float4));
            result.tangent_ws = mul((float3x3)mesh.matrix_m, result.tangent.xyz);
            result.bitangent_ws = normalize(cross(result.normal_ws, result.tangent_ws) * result.tangent.w);
        }

        if (geometry.texcoord_address != 0)
        {
            result.texcoord = vertex_buffer.Load<float2>(geometry.texcoord_address + vertex_id * sizeof(float2));
        }

        return result;
    }

    template <typename T>
    T fetch_custom_attribute(uint index)
    {
        StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];
        instance_data instance = instances[instance_id];

        StructuredBuffer<geometry_data> geometries = ResourceDescriptorHeap[scene.geometry_buffer];
        geometry_data geometry = geometries[instance.geometry_index];

        ByteAddressBuffer vertex_buffer = ResourceDescriptorHeap[scene.vertex_buffer];

        return vertex_buffer.Load<T>(geometry.custom_addresses[index] + vertex_id * sizeof(T));
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

        return mesh.matrix_m;
    }
};

#endif