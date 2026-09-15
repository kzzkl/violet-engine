#include "mesh.hlsli"
#include "brdf.hlsli"
#include "color.hlsli"
#include "material.hlsli"

struct gf2_varying
{
    float4 position_cs : SV_POSITION;
    float3 position_ws : POSITION_WS;
    float3 normal_ws : NORMAL_WS;
    float4 tangent_ws : TANGENT_WS;
    float3 bitangent_ws : BITANGENT_WS;
    float2 uv : TEXCOORD;
    float4 uv2 : TEXCOORD2;
};

float3 gf2_get_normal(gf2_varying input, float3 packed_normal)
{
    float3 tangent_normal = packed_normal * 2.0 - 1.0;

    float3 n = normalize(input.normal_ws);
    float3 t = normalize(input.tangent_ws.xyz);
    float3 b = normalize(cross(n, t));
    float3x3 tbn = transpose(float3x3(t, b, n));

    return normalize(mul(tbn, tangent_normal));
}

gf2_varying gf2_evaluate_varying(material_context context, mesh mesh)
{
    gf2_varying varying;

    float4x4 matrix_m = mesh.get_model_matrix();

    varying.position_ws = mul(matrix_m, float4(mesh.vertex.position, 1.0)).xyz;
    varying.position_cs = mul(context.camera.matrix_vp, float4(varying.position_ws, 1.0));
    varying.normal_ws = mul((float3x3)matrix_m, mesh.vertex.normal);
    varying.tangent_ws = mul(matrix_m, mesh.vertex.tangent);
    varying.bitangent_ws = normalize(cross(varying.normal_ws, varying.tangent_ws.xyz) * mesh.vertex.tangent.w);
    varying.uv = mesh.vertex.uv;
    varying.uv2 = mesh.fetch_attribute<float4>(GEOMETRY_ATTRIBUTE_CUSTOM1);

    return varying;
}

float3 gf2_evaluate_lighting(material_context context, float3 N, float3 V, float3 albedo, float roughness, float metallic, uint ramp_texture, float3 position_ws)
{
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();
    Texture2D<float4> ramp = ResourceDescriptorHeap[ramp_texture];

    StructuredBuffer<light_data> lights = ResourceDescriptorHeap[context.scene.light_buffer];

    float NdotV = saturate(dot(N, V));
    float3 F0 = lerp(0.04, albedo, metallic);

    float3 direct_lighting = 0.0;
    if (context.scene.light_count > 0)
    {
        light_data light = lights[0];

        float3 L = -light.direction;
        float3 H = normalize(L + V);

        float NdotL = saturate(dot(N, L));
        float NdotH = saturate(dot(N, H));
        float VdotH = saturate(dot(V, H));

        float d = d_ggx(NdotH, roughness);
        float vis = v_smith_joint_approx(NdotV, NdotL, roughness);
        float3 f = f_schlick(VdotH, F0);
        float3 kd = lerp(1.0 - f, 0.0, metallic);

        float3 specular = d * vis * f;
        float3 diffuse = albedo / PI * kd;
        
        float3 diffuse_ramp = ramp.SampleLevel(linear_clamp_sampler, float2(NdotL, 0.875), 0.0).rgb;
        float3 specular_ramp = ramp.SampleLevel(linear_clamp_sampler, float2(NdotL, 0.625), 0.0).rgb;

        float shadow_factor = 1.0;
        // if (light.vsm_address != 0xFFFFFFFF)
        {
            // shadow_context shadow = shadow_context::create(scene, camera);
            // shadow_factor = shadow.get_shadow(light, position_ws);
        }

        direct_lighting += (specular * specular_ramp + diffuse * diffuse_ramp) * NdotL * light.color * shadow_factor;
    }

    return direct_lighting;
}