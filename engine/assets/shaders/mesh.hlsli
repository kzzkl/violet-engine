#ifndef MESH_HLSLI
#define MESH_HLSLI

#include "common.hlsli"

#ifndef USE_RASTER_INTERPOLATION
#define USE_RASTER_INTERPOLATION 1
#endif

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

struct mesh_vertex
{
    float3 position;
    float3 normal;
    float4 tangent;
    float2 uv;
};

struct mesh
{
    geometry_data geometry;
    instance_data instance;
    mesh_data data;

    uint vertex_buffer_id;
    mesh_vertex vertex;

#if USE_RASTER_INTERPOLATION
    uint vertex_id;

    static mesh create(uint instance_id, scene_data scene, uint vertex_id);
#else
    uint3 triangle_indexes;
    barycentric_deriv deriv;

    static mesh create(uint instance_id, scene_data scene, uint primitive_id, float2 coord, float2 extent, float4x4 matrix_vp, out float2 ddx, out float2 ddy);
#endif

    template <typename T>
    T fetch_attribute(geometry_attribute attribute)
    {
        if (geometry.attributes[attribute] == 0xFFFFFFFF)
        {
            return (T)0;
        }

        ByteAddressBuffer vertex_buffer = ResourceDescriptorHeap[vertex_buffer_id];

#if USE_RASTER_INTERPOLATION
        return vertex_buffer.Load<T>(geometry.attributes[attribute] + vertex_id * sizeof(T));
#else
        return vertex_buffer.Load<T>(geometry.attributes[attribute] + triangle_indexes.x * sizeof(T));
#endif
    }

    uint get_material_address()
    {
        return instance.material_address;
    }

    float4x4 get_model_matrix()
    {
        return data.matrix_m;
    }
};

#if !USE_RASTER_INTERPOLATION
template <>
float mesh::fetch_attribute<float>(geometry_attribute attribute)
{
    if (geometry.attributes[attribute] == 0)
    {
        return (float)0;
    }

    ByteAddressBuffer vertex_buffer = ResourceDescriptorHeap[vertex_buffer_id];

    float p0 = vertex_buffer.Load<float>(geometry.attributes[attribute] + triangle_indexes.x * sizeof(float));
    float p1 = vertex_buffer.Load<float>(geometry.attributes[attribute] + triangle_indexes.y * sizeof(float));
    float p2 = vertex_buffer.Load<float>(geometry.attributes[attribute] + triangle_indexes.z * sizeof(float));

    return interpolate_with_deriv(deriv, p0, p1, p2).x;
}

template <>
float2 mesh::fetch_attribute<float2>(geometry_attribute attribute)
{
    if (geometry.attributes[attribute] == 0)
    {
        return (float2)0;
    }

    ByteAddressBuffer vertex_buffer = ResourceDescriptorHeap[vertex_buffer_id];

    float2 p0 = vertex_buffer.Load<float2>(geometry.attributes[attribute] + triangle_indexes.x * sizeof(float2));
    float2 p1 = vertex_buffer.Load<float2>(geometry.attributes[attribute] + triangle_indexes.y * sizeof(float2));
    float2 p2 = vertex_buffer.Load<float2>(geometry.attributes[attribute] + triangle_indexes.z * sizeof(float2));

    float2 result;
    result.x = interpolate_with_deriv(deriv, p0.x, p1.x, p2.x).x;
    result.y = interpolate_with_deriv(deriv, p0.y, p1.y, p2.y).x;
    return result;
}

template <>
float3 mesh::fetch_attribute<float3>(geometry_attribute attribute)
{
    if (geometry.attributes[attribute] == 0)
    {
        return (float3)0;
    }

    ByteAddressBuffer vertex_buffer = ResourceDescriptorHeap[vertex_buffer_id];

    float3 p0 = vertex_buffer.Load<float3>(geometry.attributes[attribute] + triangle_indexes.x * sizeof(float3));
    float3 p1 = vertex_buffer.Load<float3>(geometry.attributes[attribute] + triangle_indexes.y * sizeof(float3));
    float3 p2 = vertex_buffer.Load<float3>(geometry.attributes[attribute] + triangle_indexes.z * sizeof(float3));

    float3 result;
    result.x = interpolate_with_deriv(deriv, p0.x, p1.x, p2.x).x;
    result.y = interpolate_with_deriv(deriv, p0.y, p1.y, p2.y).x;
    result.z = interpolate_with_deriv(deriv, p0.z, p1.z, p2.z).x;
    
    return result;
}

template <>
float4 mesh::fetch_attribute<float4>(geometry_attribute attribute)
{
    if (geometry.attributes[attribute] == 0)
    {
        return (float4)0;
    }

    ByteAddressBuffer vertex_buffer = ResourceDescriptorHeap[vertex_buffer_id];

    float4 p0 = vertex_buffer.Load<float4>(geometry.attributes[attribute] + triangle_indexes.x * sizeof(float4));
    float4 p1 = vertex_buffer.Load<float4>(geometry.attributes[attribute] + triangle_indexes.y * sizeof(float4));
    float4 p2 = vertex_buffer.Load<float4>(geometry.attributes[attribute] + triangle_indexes.z * sizeof(float4));

    float4 result;
    result.x = interpolate_with_deriv(deriv, p0.x, p1.x, p2.x).x;
    result.y = interpolate_with_deriv(deriv, p0.y, p1.y, p2.y).x;
    result.z = interpolate_with_deriv(deriv, p0.z, p1.z, p2.z).x;
    result.w = interpolate_with_deriv(deriv, p0.w, p1.w, p2.w).x;

    return result;
}
#endif

#if USE_RASTER_INTERPOLATION
mesh mesh::create(uint instance_id, scene_data scene, uint vertex_id)
{
    mesh mesh;

    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];
    mesh.instance = instances[instance_id];

    StructuredBuffer<geometry_data> geometries = ResourceDescriptorHeap[scene.geometry_buffer];
    mesh.geometry = geometries[mesh.instance.submesh_id];

    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
    mesh.data = meshes[mesh.instance.mesh_id];

    mesh.vertex_buffer_id = scene.vertex_buffer;
    mesh.vertex_id = vertex_id;

    mesh.vertex.position = mesh.fetch_attribute<float3>(GEOMETRY_ATTRIBUTE_POSITION);
    mesh.vertex.normal = mesh.fetch_attribute<float3>(GEOMETRY_ATTRIBUTE_NORMAL);
    mesh.vertex.tangent = mesh.fetch_attribute<float4>(GEOMETRY_ATTRIBUTE_TANGENT);
    mesh.vertex.uv = mesh.fetch_attribute<float2>(GEOMETRY_ATTRIBUTE_UV);

    return mesh;
}
#else
mesh mesh::create(uint instance_id, scene_data scene, uint primitive_id, float2 coord, float2 extent, float4x4 matrix_vp, out float2 ddx, out float2 ddy)
{
    mesh mesh;

    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];
    mesh.instance = instances[instance_id];

    StructuredBuffer<geometry_data> geometries = ResourceDescriptorHeap[scene.geometry_buffer];
    mesh.geometry = geometries[mesh.instance.submesh_id];

    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
    mesh.data = meshes[mesh.instance.mesh_id];

    mesh.vertex_buffer_id = scene.vertex_buffer;

    ByteAddressBuffer vertex_buffer = ResourceDescriptorHeap[scene.vertex_buffer];
    StructuredBuffer<uint> index_buffer = ResourceDescriptorHeap[scene.index_buffer];

    uint index_offset = mesh.geometry.index_offset + primitive_id * 3;
    mesh.triangle_indexes = uint3(
        index_buffer[index_offset],
        index_buffer[index_offset + 1],
        index_buffer[index_offset + 2]);

    float4x4 matrix_m = mesh.get_model_matrix();

    float3 p0 = vertex_buffer.Load<float3>(mesh.geometry.attributes[GEOMETRY_ATTRIBUTE_POSITION] + mesh.triangle_indexes.x * sizeof(float3));
    float4 p0_cs = mul(matrix_vp, mul(matrix_m, float4(p0, 1.0)));

    float3 p1 = vertex_buffer.Load<float3>(mesh.geometry.attributes[GEOMETRY_ATTRIBUTE_POSITION] + mesh.triangle_indexes.y * sizeof(float3));
    float4 p1_cs = mul(matrix_vp, mul(matrix_m, float4(p1, 1.0)));

    float3 p2 = vertex_buffer.Load<float3>(mesh.geometry.attributes[GEOMETRY_ATTRIBUTE_POSITION] + mesh.triangle_indexes.z * sizeof(float3));
    float4 p2_cs = mul(matrix_vp, mul(matrix_m, float4(p2, 1.0)));

    float2 pixel_ndc = (coord + 0.5) / extent * float2(2.0, -2.0) + float2(-1.0, 1.0);
    mesh.deriv = calculate_full_bary(p0_cs, p1_cs, p2_cs, pixel_ndc, extent);

    mesh.vertex.position.x = interpolate_with_deriv(mesh.deriv, p0.x, p1.x, p2.x).x;
    mesh.vertex.position.y = interpolate_with_deriv(mesh.deriv, p0.y, p1.y, p2.y).x;
    mesh.vertex.position.z = interpolate_with_deriv(mesh.deriv, p0.z, p1.z, p2.z).x;

    mesh.vertex.normal = mesh.fetch_attribute<float3>(GEOMETRY_ATTRIBUTE_NORMAL);
    mesh.vertex.tangent = mesh.fetch_attribute<float4>(GEOMETRY_ATTRIBUTE_TANGENT);

    if (mesh.geometry.attributes[GEOMETRY_ATTRIBUTE_UV] != 0)
    {
        float2 p0_uv = vertex_buffer.Load<float2>(mesh.geometry.attributes[GEOMETRY_ATTRIBUTE_UV] + mesh.triangle_indexes.x * sizeof(float2));
        float2 p1_uv = vertex_buffer.Load<float2>(mesh.geometry.attributes[GEOMETRY_ATTRIBUTE_UV] + mesh.triangle_indexes.y * sizeof(float2));
        float2 p2_uv = vertex_buffer.Load<float2>(mesh.geometry.attributes[GEOMETRY_ATTRIBUTE_UV] + mesh.triangle_indexes.z * sizeof(float2));

        float3 interpolate_x = interpolate_with_deriv(mesh.deriv, p0_uv.x, p1_uv.x, p2_uv.x);
        float3 interpolate_y = interpolate_with_deriv(mesh.deriv, p0_uv.y, p1_uv.y, p2_uv.y);

        mesh.vertex.uv.x = interpolate_x.x;
        mesh.vertex.uv.y = interpolate_y.x;

        ddx = float2(interpolate_x.y, interpolate_y.y);
        ddy = float2(interpolate_x.z, interpolate_y.z);
    }

    return mesh;
}
#endif

#endif