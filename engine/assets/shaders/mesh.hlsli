#ifndef MESH_HLSLI
#define MESH_HLSLI

#include "common.hlsli"

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

// http://filmicworlds.com/blog/visibility-buffer-rendering-with-material-graphs/
struct barycentric_deriv
{
    float3 lambda;
    float3 ddx;
    float3 ddy;
};

barycentric_deriv calculate_full_bary(float4 pt0, float4 pt1, float4 pt2, float2 ndc, float2 extent)
{
    barycentric_deriv ret = (barycentric_deriv)0;

    float3 inv_w = rcp(float3(pt0.w, pt1.w, pt2.w));

    float2 ndc0 = pt0.xy * inv_w.x;
    float2 ndc1 = pt1.xy * inv_w.y;
    float2 ndc2 = pt2.xy * inv_w.z;

    float inv_det = rcp(determinant(float2x2(ndc2 - ndc1, ndc0 - ndc1)));
    ret.ddx = float3(ndc1.y - ndc2.y, ndc2.y - ndc0.y, ndc0.y - ndc1.y) * inv_det * inv_w;
    ret.ddy = float3(ndc2.x - ndc1.x, ndc0.x - ndc2.x, ndc1.x - ndc0.x) * inv_det * inv_w;
    float ddx_sum = dot(ret.ddx, float3(1, 1, 1));
    float ddy_sum = dot(ret.ddy, float3(1, 1, 1));

    float2 delta_vec = ndc - ndc0;
    float interp_inv_w = inv_w.x + delta_vec.x * ddx_sum + delta_vec.y * ddy_sum;
    float interp_w = rcp(interp_inv_w);

    ret.lambda.x = interp_w * (inv_w[0] + delta_vec.x * ret.ddx.x + delta_vec.y * ret.ddy.x);
    ret.lambda.y = interp_w * (0.0f + delta_vec.x * ret.ddx.y + delta_vec.y * ret.ddy.y);
    ret.lambda.z = interp_w * (0.0f + delta_vec.x * ret.ddx.z + delta_vec.y * ret.ddy.z);

    ret.ddx *= 2.0 / extent.x;
    ret.ddy *= 2.0 / extent.y;
    ddx_sum *= 2.0 / extent.x;
    ddy_sum *= 2.0 / extent.y;

    ret.ddy *= -1.0;
    ddy_sum *= -1.0;

    float interp_w_ddx = 1.0 / (interp_inv_w + ddx_sum);
    float interp_w_ddy = 1.0 / (interp_inv_w + ddy_sum);

    ret.ddx = interp_w_ddx*(ret.lambda*interp_inv_w + ret.ddx) - ret.lambda;
    ret.ddy = interp_w_ddy*(ret.lambda*interp_inv_w + ret.ddy) - ret.lambda;  

    return ret;
}

float3 interpolate_with_deriv(barycentric_deriv deriv, float v0, float v1, float v2)
{
    const float3 merged_v = float3(v0, v1, v2);
    float3 ret;
    ret.x = dot(merged_v, deriv.lambda);
    ret.y = dot(merged_v, deriv.ddx);
    ret.z = dot(merged_v, deriv.ddy);
    return ret;
}

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

    static mesh create(uint instance_id)
    {
        mesh result;
        result.instance_id = instance_id;
        return result;
    }

    vertex fetch_vertex(uint vertex_id)
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

    vertex fetch_vertex(uint primitive_id, float2 coord, float2 extent, out float2 ddx, out float2 ddy)
    {
        StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];
        instance_data instance = instances[instance_id];

        StructuredBuffer<geometry_data> geometries = ResourceDescriptorHeap[scene.geometry_buffer];
        geometry_data geometry = geometries[instance.geometry_index];

        StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
        mesh_data mesh = meshes[instance.mesh_index];

        ByteAddressBuffer vertex_buffer = ResourceDescriptorHeap[scene.vertex_buffer];
        StructuredBuffer<uint> index_buffer = ResourceDescriptorHeap[scene.index_buffer];

        uint index_offset = geometry.index_offset + primitive_id * 3;
        uint3 triangle_indexes = uint3(
            index_buffer[index_offset],
            index_buffer[index_offset + 1],
            index_buffer[index_offset + 2]);

        float3 p0 = vertex_buffer.Load<float3>(geometry.position_address + triangle_indexes.x * sizeof(float3));
        float4 p0_cs = mul(camera.matrix_vp, mul(mesh.matrix_m, float4(p0, 1.0)));

        float3 p1 = vertex_buffer.Load<float3>(geometry.position_address + triangle_indexes.y * sizeof(float3));
        float4 p1_cs = mul(camera.matrix_vp, mul(mesh.matrix_m, float4(p1, 1.0)));

        float3 p2 = vertex_buffer.Load<float3>(geometry.position_address + triangle_indexes.z * sizeof(float3));
        float4 p2_cs = mul(camera.matrix_vp, mul(mesh.matrix_m, float4(p2, 1.0)));

        float2 pixel_ndc = (coord + 0.5) / extent * float2(2.0, -2.0) + float2(-1.0, 1.0);
        barycentric_deriv deriv = calculate_full_bary(p0_cs, p1_cs, p2_cs, pixel_ndc, extent);

        vertex result;
        result.position.x = interpolate_with_deriv(deriv, p0.x, p1.x, p2.x).x;
        result.position.y = interpolate_with_deriv(deriv, p0.y, p1.y, p2.y).x;
        result.position.z = interpolate_with_deriv(deriv, p0.z, p1.z, p2.z).x;
        result.position_ws = mul(mesh.matrix_m, float4(result.position, 1.0)).xyz;
        result.position_cs = mul(camera.matrix_vp, float4(result.position_ws, 1.0));

        if (geometry.normal_address != 0)
        {
            float3 p0_normal = vertex_buffer.Load<float3>(geometry.normal_address + triangle_indexes.x * sizeof(float3));
            float3 p1_normal = vertex_buffer.Load<float3>(geometry.normal_address + triangle_indexes.y * sizeof(float3));
            float3 p2_normal = vertex_buffer.Load<float3>(geometry.normal_address + triangle_indexes.z * sizeof(float3));

            result.normal.x = interpolate_with_deriv(deriv, p0_normal.x, p1_normal.x, p2_normal.x).x;
            result.normal.y = interpolate_with_deriv(deriv, p0_normal.y, p1_normal.y, p2_normal.y).x;
            result.normal.z = interpolate_with_deriv(deriv, p0_normal.z, p1_normal.z, p2_normal.z).x;
            result.normal_ws = normalize(mul((float3x3)mesh.matrix_m, result.normal));
        }

        if (geometry.tangent_address!= 0)
        {
            float4 p0_tangent = vertex_buffer.Load<float4>(geometry.tangent_address + triangle_indexes.x * sizeof(float4));
            float4 p1_tangent = vertex_buffer.Load<float4>(geometry.tangent_address + triangle_indexes.y * sizeof(float4));
            float4 p2_tangent = vertex_buffer.Load<float4>(geometry.tangent_address + triangle_indexes.z * sizeof(float4));

            result.tangent.x = interpolate_with_deriv(deriv, p0_tangent.x, p1_tangent.x, p2_tangent.x).x;
            result.tangent.y = interpolate_with_deriv(deriv, p0_tangent.y, p1_tangent.y, p2_tangent.y).x;
            result.tangent.z = interpolate_with_deriv(deriv, p0_tangent.z, p1_tangent.z, p2_tangent.z).x;
            result.tangent_ws = mul((float3x3)mesh.matrix_m, result.tangent.xyz);
            result.bitangent_ws = normalize(cross(result.normal_ws, result.tangent_ws) * result.tangent.w);
        }

        if (geometry.texcoord_address != 0)
        {
            float2 p0_texcoord = vertex_buffer.Load<float2>(geometry.texcoord_address + triangle_indexes.x * sizeof(float2));
            float2 p1_texcoord = vertex_buffer.Load<float2>(geometry.texcoord_address + triangle_indexes.y * sizeof(float2));
            float2 p2_texcoord = vertex_buffer.Load<float2>(geometry.texcoord_address + triangle_indexes.z * sizeof(float2));

            float3 interpolate_x = interpolate_with_deriv(deriv, p0_texcoord.x, p1_texcoord.x, p2_texcoord.x);
            float3 interpolate_y = interpolate_with_deriv(deriv, p0_texcoord.y, p1_texcoord.y, p2_texcoord.y);

            result.texcoord.x = interpolate_x.x;
            result.texcoord.y = interpolate_y.x;

            ddx.x = interpolate_x.y;
            ddx.y = interpolate_x.y;
            ddy.x = interpolate_y.y;
            ddy.y = interpolate_y.y;
        }

        return result;
    }

    template <typename T>
    T fetch_custom_attribute(uint vertex_id, uint attribute_index)
    {
        StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];
        instance_data instance = instances[instance_id];

        StructuredBuffer<geometry_data> geometries = ResourceDescriptorHeap[scene.geometry_buffer];
        geometry_data geometry = geometries[instance.geometry_index];

        ByteAddressBuffer vertex_buffer = ResourceDescriptorHeap[scene.vertex_buffer];

        return vertex_buffer.Load<T>(geometry.custom_addresses[attribute_index] + vertex_id * sizeof(T));
    }

    uint get_material_address()
    {
        StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];
        return instances[instance_id].material_address;
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