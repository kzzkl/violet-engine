#ifndef PBR_MATERIAL_HLSLI
#define PBR_MATERIAL_HLSLI

#include "material.hlsli"

struct pbr_material
{
    struct varying
    {
        float4 position_cs : SV_POSITION;
        float3 position_ws : POSITION;
        float3 normal_ws : NORMAL;
        float4 tangent_ws : TANGENT;
        float2 uv : TEXCOORD0;
    };

    float3 albedo;
    uint albedo_texture;
    float roughness;
    uint roughness_metallic_texture;
    float metallic;
    uint normal_texture;
    float3 emissive;
    uint emissive_texture;

    varying evaluate_varying(material_context context, mesh mesh)
    {
        varying varying;

        float4x4 matrix_m = mesh.get_model_matrix();

        float3 position = mesh.fetch_attribute<float3>(GEOMETRY_ATTRIBUTE_POSITION);
        varying.position_ws = mul(matrix_m, float4(position, 1.0)).xyz;
        varying.position_cs = mul(context.camera.matrix_vp, float4(varying.position_ws, 1.0));

        float3 normal = mesh.fetch_attribute<float3>(GEOMETRY_ATTRIBUTE_NORMAL);
        varying.normal_ws = mul((float3x3)matrix_m, normal);

        float4 tangent = mesh.fetch_attribute<float4>(GEOMETRY_ATTRIBUTE_TANGENT);
        varying.tangent_ws = mul(matrix_m, tangent);

        varying.uv = mesh.fetch_attribute<float2>(GEOMETRY_ATTRIBUTE_UV);

        return varying;
    }

    surface evaluate_surface(material_context context, varying varying)
    {
        surface surface;

        SamplerState linear_repeat_sampler = get_linear_repeat_sampler();

        Texture2D<float3> albedo_tex = ResourceDescriptorHeap[albedo_texture];
        Texture2D<float3> roughness_metallic_tex = ResourceDescriptorHeap[roughness_metallic_texture];
        float3 roughness_metallic = context.sample_texture(roughness_metallic_tex, linear_repeat_sampler, varying.uv);

        Texture2D<float3> emissive_tex = ResourceDescriptorHeap[emissive_texture];

        if (normal_texture != 0)
        {
            Texture2D<float3> normal_tex = ResourceDescriptorHeap[normal_texture];

            float3 tangent_normal = normalize(context.sample_texture(normal_tex, linear_repeat_sampler, varying.uv) * 2.0 - 1.0);
            float3 n = normalize(varying.normal_ws);
            float3 t = normalize(varying.tangent_ws.xyz);
            float3 b = normalize(cross(n, t)) * varying.tangent_ws.w;
            float3x3 tbn = transpose(float3x3(t, b, n));
            surface.normal_ws = normalize(mul(tbn, tangent_normal));
        }
        else
        {
            surface.normal_ws = normalize(varying.normal_ws);
        }

        surface.albedo = albedo * context.sample_texture(albedo_tex, linear_repeat_sampler, varying.uv);
        surface.opacity = 1.0;
        surface.roughness = roughness * roughness_metallic.g;
        surface.metallic = metallic * roughness_metallic.b;
        surface.emissive = emissive * context.sample_texture(emissive_tex, linear_repeat_sampler, varying.uv);

        return surface;
    }
};

#endif