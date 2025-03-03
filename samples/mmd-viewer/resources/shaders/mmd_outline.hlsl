#include "common.hlsli"

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

float3 outline_offset(float3 position_ws, float3 normal_ws, float outline_width)
{
    float4 position_vs = mul(camera.view, float4(position_ws, 1.0));

    float camera_mul_fix = abs(position_vs.z);
    camera_mul_fix = saturate(camera_mul_fix);
    camera_mul_fix *= camera.fov;
    camera_mul_fix *= 0.001;
    
    float outline_expand = outline_width * camera_mul_fix;
    position_ws.xyz += normal_ws * outline_expand;

    return position_ws.xyz + normal_ws * outline_expand;
}

struct vs_input
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 smooth_normal : SMOOTH_NORMAL;
    float outline: OUTLINE;
};

struct vs_output
{
    float4 position_cs : SV_POSITION;
    float3 color : COLOR;
};

struct mmd_outline
{
    float3 color;
    float width;
    float z_offset;
    float strength;
};

float3 get_smooth_normal(vs_input input)
{
    float3 n = normalize(input.normal);
    float3 t = normalize(input.tangent);
    float3 b = normalize(cross(n, t));
    float3x3 tbn = transpose(float3x3(t, b, n));

    return normalize(mul(tbn, input.smooth_normal));
}

vs_output vs_main(vs_input input, uint instance_index : SV_InstanceID)
{
    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];

    instance_data instance = instances[instance_index];
    mesh_data mesh = meshes[instance.mesh_index];
    
    mmd_outline material = load_material<mmd_outline>(scene.material_buffer, instance.material_address);

    float4 position_ws = mul(mesh.model_matrix, float4(input.position, 1.0));
    float3 smooth_normal_ws = mul((float3x3)mesh.model_matrix, get_smooth_normal(input));

    float4 position_vs = mul(camera.view, position_ws);

    // https://github.com/ColinLeung-NiloCat/UnityURPToonLitShaderExample/blob/master/NiloOutlineUtil.hlsl
    float camera_mul_fix = abs(position_vs.z);
    camera_mul_fix = saturate(camera_mul_fix);
    camera_mul_fix *= camera.fov / PI * 180.0;
    camera_mul_fix *= 0.001;
    position_ws.xyz += smooth_normal_ws * material.width * input.outline * camera_mul_fix;

    // z offset.
    position_vs = mul(camera.view, float4(position_ws.xyz, 1.0));
    position_vs.xyz += normalize(position_vs.xyz) * material.z_offset;

    vs_output output;
    output.position_cs = mul(camera.projection, position_vs);
    output.color = material.color * material.strength;

    return output;
}

struct fs_output
{
    float4 albedo : SV_TARGET0;
    float2 material : SV_TARGET1;
    float2 normal : SV_TARGET2;
    float4 emissive : SV_TARGET3;
};

fs_output fs_main(vs_output input)
{
    fs_output output;
    output.albedo = float4(input.color, 1.0);
    output.material = 0.0;
    output.normal = 0.0;
    output.emissive = 0.0;

    return output;
}