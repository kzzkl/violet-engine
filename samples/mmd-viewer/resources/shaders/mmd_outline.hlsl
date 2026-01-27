#include "mesh.hlsli"
#include "shading/shading_model.hlsli"

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
    float3 color : COLOR;
    uint shading_model : SHADING_MODEL;
};

struct mmd_outline_material
{
    float3 color;
    float width;
    float z_offset;
    float strength;
};

float3 get_smooth_normal(vertex vertex, float3 smooth_normal)
{
    float3 n = normalize(vertex.normal);
    float3 t = normalize(vertex.tangent.xyz);
    float3 b = normalize(cross(n, t) * vertex.tangent.w);
    float3x3 tbn = transpose(float3x3(t, b, n));

    return normalize(mul(tbn, smooth_normal));
}

vs_output vs_main(uint vertex_id : SV_VertexID, uint draw_id : SV_InstanceID)
{
    StructuredBuffer<draw_info> draw_infos = ResourceDescriptorHeap[constant.draw_info_buffer];
    uint instance_id = draw_infos[draw_id].instance_id;

    mesh mesh = mesh::create(instance_id, scene);
    vertex vertex = mesh.fetch_vertex(vertex_id, camera.matrix_vp);
    
    mmd_outline_material material = load_material<mmd_outline_material>(scene.material_buffer, mesh.get_material_address());
    material_info material_info = load_material_info(scene.material_buffer, mesh.get_material_address());

    float4 smooth_normal_and_outline = mesh.fetch_custom_attribute<float4>(vertex_id, 0);
    float3 smooth_normal_ws = mul((float3x3)mesh.get_model_matrix(), get_smooth_normal(vertex, smooth_normal_and_outline.xyz));

    float4 position_vs = mul(camera.matrix_v, float4(vertex.position_ws, 1.0));

    // https://github.com/ColinLeung-NiloCat/UnityURPToonLitShaderExample/blob/master/NiloOutlineUtil.hlsl
    float camera_mul_fix = abs(position_vs.z);
    camera_mul_fix = saturate(camera_mul_fix);
    camera_mul_fix *= camera.fov / PI * 180.0;
    camera_mul_fix *= 0.001;
    vertex.position_ws += smooth_normal_ws * material.width *  camera_mul_fix;

    // z offset.
    position_vs = mul(camera.matrix_v, float4(vertex.position_ws, 1.0));
    position_vs.xyz += normalize(position_vs.xyz) * material.z_offset;

    vs_output output;
    output.position_cs = mul(camera.matrix_p, position_vs);
    output.color = material.color * material.strength;
    output.shading_model = material_info.shading_model;

    return output;
}

struct fs_output
{
    float4 albedo : SV_TARGET0;
    float2 material : SV_TARGET1;
    uint normal : SV_TARGET2;
    float4 emissive : SV_TARGET3;
};

fs_output fs_main(vs_output input)
{
    fs_output output;
    output.albedo = float4(input.color, 1.0);
    output.material = 0.0;
    output.normal = pack_gbuffer_normal(float3(0.0, 0.0, 0.0), input.shading_model);
    output.emissive = 0.0;

    return output;
}