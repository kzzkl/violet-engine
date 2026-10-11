#include "gf2/gf2_material.hlsli"

struct gf2_material_face
{
    using varying = gf2_varying;

    uint diffuse_texture_id;
    uint sdf_texture_id;
    uint ramp_texture_id;
    uint brdf_lut;
    float3 face_front_dir;
    uint padding0;
    float3 face_left_dir;
    uint padding1;

    varying evaluate_varying(material_context context, mesh mesh)
    {
        return gf2_evaluate_varying(context, mesh);
    }

    surface evaluate_surface(material_context context, varying varying)
    {
        SamplerState linear_repeat_sampler = get_linear_repeat_sampler();
        SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

        Texture2D<float4> diffuse_texture = ResourceDescriptorHeap[diffuse_texture_id];
        Texture2D<float4> sdf_texture = ResourceDescriptorHeap[sdf_texture_id];
        Texture2D<float4> ramp_texture = ResourceDescriptorHeap[ramp_texture_id];

        float3 albedo = context.sample_texture(diffuse_texture, linear_repeat_sampler, varying.uv).rgb;
        float roughness = 1.0;
        float metallic = 0.0;

        float3 V = normalize(context.camera.position - varying.position_ws);
        float3 N = normalize(varying.normal_ws);

        StructuredBuffer<light_data> lights = ResourceDescriptorHeap[context.scene.light_buffer];

        float NdotV = saturate(dot(N, V));
        float3 F0 = lerp(0.04, albedo, metallic);

        float3 lighting = 0.0;
        if (context.scene.light_count > 0)
        {
            light_data light = lights[0];

            float3 L = -light.direction;
            float3 H = normalize(L + V);

            float NdotL = 1.0;
            float NdotH = 1.0;
            float VdotH = 1.0;

            float d = d_ggx(NdotH, roughness);
            float vis = v_smith_joint_approx(NdotV, NdotL, roughness);
            float3 f = f_schlick(VdotH, F0);
            float3 kd = lerp(1.0 - f, 0.0, metallic);

            float3 specular = d * vis * f;
            float3 diffuse = albedo / PI * kd;

            float3 diffuse_ramp = context.sample_texture(ramp_texture, linear_clamp_sampler, float2(NdotL, 0.875), 0.0).rgb;
            float3 specular_ramp = context.sample_texture(ramp_texture, linear_clamp_sampler, float2(NdotL, 0.625), 0.0).rgb;

            float2 projected_light = normalize(L.xz);
            float2 front_dir = normalize(face_front_dir.xz);
            float2 left_dir = normalize(face_left_dir.xz);
            float threshold = dot(front_dir, normalize(L.xz));
            threshold = 1.0 - (threshold * 0.5 + 0.5);

            float sign = dot(left_dir, projected_light) > 0.0 ? 1.0 : -1.0;
            float4 sdf = context.sample_texture(sdf_texture, linear_repeat_sampler, float2(varying.uv2.x * sign, varying.uv2.y), 0.0);
            float shadow = step(threshold, sdf.r);

            lighting += (specular * specular_ramp + diffuse * diffuse_ramp) * light.color * shadow;
        }

        surface surface;
        surface.albedo = 0.0;
        surface.roughness = 0.0;
        surface.metallic = 0.0;
        surface.emissive = lighting;
        surface.normal_ws = N;

        return surface;
    }
};