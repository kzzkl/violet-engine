#include "mesh.hlsli"
#include "gbuffer.hlsli"

struct unlit_material
{
    float3 albedo;

    gbuffer resolve(vertex vertex, float2 ddx, float2 ddy)
    {
        gbuffer gbuffer;
        gbuffer.albedo = albedo;
        gbuffer.roughness = 0.0;
        gbuffer.metallic = 0.0;
        gbuffer.emissive = 0.0;
        gbuffer.normal = vertex.normal;
        return gbuffer;
    }

    gbuffer resolve(float3 normal_ws)
    {
        gbuffer gbuffer;
        gbuffer.albedo = albedo;
        gbuffer.roughness = 0.0;
        gbuffer.metallic = 0.0;
        gbuffer.emissive = 0.0;
        gbuffer.normal = normal_ws;
        return gbuffer;
    }
};

#ifdef VIOLET_MATERIAL_PATH_DEFERRED
struct constant_data
{
    uint draw_info_buffer;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

struct vs_output
{
    float4 position_cs : SV_POSITION;
    float3 normal_ws : NORMAL;
    uint material_address : MATERIAL_ADDRESS;

    uint shading_model : SHADING_MODEL;
};

vs_output vs_main(uint vertex_id : SV_VertexID, uint draw_id : SV_InstanceID)
{
    StructuredBuffer<draw_info> draw_infos = ResourceDescriptorHeap[constant.draw_info_buffer];
    uint instance_id = draw_infos[draw_id].instance_id;

    mesh mesh = mesh::create(instance_id, scene);
    vertex vertex = mesh.fetch_vertex(vertex_id, camera.matrix_vp);

    vs_output output;
    output.position_cs = vertex.position_cs;
    output.normal_ws = vertex.normal_ws;
    output.material_address = mesh.get_material_address();

    material_info material_info = load_material_info(scene.material_buffer, output.material_address);
    output.shading_model = material_info.shading_model;

    return output;
}

fs_output fs_main(vs_output input)
{
    unlit_material material = load_material<unlit_material>(scene.material_buffer, input.material_address);
    gbuffer gbuffer = material.resolve(input.normal_ws);
    gbuffer.shading_model = input.shading_model;

    return fs_output::create(gbuffer);
}
#else
#include "visibility/material_resolve.hlsli"

[shader("compute")]
[numthreads(8, 8, 1)]
void cs_main(uint3 gtid : SV_GroupThreadID, uint3 gid : SV_GroupID)
{
    resolve_material<unlit_material>(gtid, gid);
}
#endif