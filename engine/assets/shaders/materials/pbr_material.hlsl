#include "mesh.hlsli"
#include "gbuffer.hlsli"

struct pbr_material
{
    float3 albedo;
    uint albedo_texture;
    float roughness;
    uint roughness_metallic_texture;
    float metallic;
    uint normal_texture;
    float3 emissive;
    uint emissive_texture;

    gbuffer resolve(float3 normal_ws, float3 tangent_ws, float2 texcoord)
    {
        SamplerState linear_repeat_sampler = get_linear_repeat_sampler();

        Texture2D<float3> albedo_tex = ResourceDescriptorHeap[albedo_texture];
        Texture2D<float3> roughness_metallic_tex = ResourceDescriptorHeap[roughness_metallic_texture];
        float3 roughness_metallic = roughness_metallic_tex.Sample(linear_repeat_sampler, texcoord);

        Texture2D<float3> emissive_tex = ResourceDescriptorHeap[emissive_texture];

        float3 N = normal_ws;
        if (normal_texture != 0)
        {
            Texture2D<float3> normal_tex = ResourceDescriptorHeap[normal_texture];

            float3 tangent_normal = normalize(normal_tex.Sample(linear_repeat_sampler, texcoord) * 2.0 - 1.0);
            float3 n = normalize(normal_ws);
            float3 t = normalize(tangent_ws);
            float3 b = normalize(cross(n, t));
            float3x3 tbn = transpose(float3x3(t, b, n));
            N = normalize(mul(tbn, tangent_normal));
        }

        gbuffer gbuffer;
        gbuffer.albedo = albedo * albedo_tex.Sample(linear_repeat_sampler, texcoord);
        gbuffer.roughness = roughness * roughness_metallic.g;
        gbuffer.metallic = metallic * roughness_metallic.b;
        gbuffer.emissive = emissive * emissive_tex.Sample(linear_repeat_sampler, texcoord);
        gbuffer.normal = N;
        return gbuffer;
    }

    gbuffer resolve(vertex vertex, float2 ddx, float2 ddy)
    {
        SamplerState linear_repeat_sampler = get_linear_repeat_sampler();

        Texture2D<float3> albedo_tex = ResourceDescriptorHeap[albedo_texture];
        Texture2D<float3> roughness_metallic_tex = ResourceDescriptorHeap[roughness_metallic_texture];
        float3 roughness_metallic = roughness_metallic_tex.SampleGrad(linear_repeat_sampler, vertex.texcoord, ddx, ddy);

        Texture2D<float3> emissive_tex = ResourceDescriptorHeap[emissive_texture];

        float3 N = vertex.normal_ws;
        if (normal_texture != 0)
        {
            Texture2D<float3> normal_tex = ResourceDescriptorHeap[normal_texture];

            float3 tangent_normal = normalize(normal_tex.SampleGrad(linear_repeat_sampler, vertex.texcoord, ddx, ddy) * 2.0 - 1.0);
            float3 n = normalize(vertex.normal_ws);
            float3 t = normalize(vertex.tangent_ws);
            float3 b = normalize(cross(n, t));
            float3x3 tbn = transpose(float3x3(t, b, n));
            N = normalize(mul(tbn, tangent_normal));
        }

        gbuffer gbuffer;
        gbuffer.albedo = albedo * albedo_tex.SampleGrad(linear_repeat_sampler, vertex.texcoord, ddx, ddy);
        gbuffer.roughness = roughness * roughness_metallic.g;
        gbuffer.metallic = metallic * roughness_metallic.b;
        gbuffer.emissive = emissive * emissive_tex.SampleGrad(linear_repeat_sampler, vertex.texcoord, ddx, ddy);
        gbuffer.normal = N;
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
    float3 tangent_ws : TANGENT;
    float2 texcoord : TEXCOORD1;
    uint material_address : MATERIAL_ADDRESS;

    uint shading_model : SHADING_MODEL;

#ifdef VIOLET_OPACITY_CUTOFF
    uint opacity_mask : OPACITY_MASK;
    uint opacity_cutoff : OPACITY_CUTOFF;
#endif
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
    output.tangent_ws = vertex.tangent_ws;
    output.texcoord = vertex.texcoord;
    output.material_address = mesh.get_material_address();
    
    material_info material_info = load_material_info(scene.material_buffer, output.material_address);
    output.shading_model = material_info.shading_model;

#ifdef VIOLET_OPACITY_CUTOFF
    output.opacity_mask = material_info.opacity_mask;
    output.opacity_cutoff = material_info.opacity_cutoff;
#endif

    return output;
}

fs_output fs_main(vs_output input)
{
#ifdef VIOLET_OPACITY_CUTOFF
    Texture2D<float4> opacity_mask = ResourceDescriptorHeap[input.opacity_mask];
    SamplerState point_repeat_sampler = get_point_repeat_sampler();

    float mask = opacity_mask.Sample(point_repeat_sampler, input.texcoord).a;
    clip(mask * 255.0 - input.opacity_cutoff);
#endif

    pbr_material material = load_material<pbr_material>(scene.material_buffer, input.material_address);
    gbuffer gbuffer = material.resolve(input.normal_ws, input.tangent_ws, input.texcoord);
    gbuffer.shading_model = input.shading_model;

    return fs_output::create(gbuffer);
}

#else
#include "visibility/material_resolve.hlsli"

[shader("compute")]
[numthreads(8, 8, 1)]
void cs_main(uint3 gtid : SV_GroupThreadID, uint3 gid : SV_GroupID)
{
    resolve_material<pbr_material>(gtid, gid);
}
#endif