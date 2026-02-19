#include "visibility/material_resolve.hlsli"

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

[numthreads(8, 8, 1)]
void cs_main(uint3 gtid : SV_GroupThreadID, uint3 gid : SV_GroupID)
{
    resolve_material<pbr_material>(gtid, gid);
}