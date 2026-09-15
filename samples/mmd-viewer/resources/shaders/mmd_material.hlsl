#include "color.hlsli"
#include "brdf.hlsli"
#include "material.hlsli"

float3 multiply_rgb(float3 a, float3 b, float factor)
{
    return (1.0 - factor) * a + factor * a * b;
}

float3 screen_rgb(float3 a, float3 b, float factor)
{
    return (1.0 - factor) * a + factor * (1.0 - (1.0 - a) * (1.0 - b));
}

struct mmd_material
{
    struct varying
    {
        float4 position_cs : SV_POSITION;
        float3 normal_ws : NORMAL_WS;
        float2 uv : TEXCOORD;
    };

    float4 diffuse;
    float3 specular;
    float specular_strength;
    float3 ambient;
    uint diffuse_texture_id;
    uint toon_texture_id;
    uint environment_texture_id;
    uint environment_blend_mode;
    uint ramp_texture_id;

    varying evaluate_varying(material_context context, mesh mesh)
    {
        varying varying;

        float4x4 matrix_m = mesh.get_model_matrix();

        varying.position_cs = mul(context.camera.matrix_vp, mul(matrix_m, float4(mesh.vertex.position, 1.0)));
        varying.normal_ws = mul((float3x3)matrix_m, mesh.vertex.normal);
        varying.uv = mesh.vertex.uv;

        return varying;
    }

    surface evaluate_surface(material_context context, varying varying)
    {
        SamplerState linear_repeat_sampler = get_linear_repeat_sampler();
        SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

        Texture2D<float4> diffuse_texture = ResourceDescriptorHeap[diffuse_texture_id];
        Texture2D<float4> environment_texture = ResourceDescriptorHeap[environment_texture_id];

        float4 color = diffuse * context.sample_texture(diffuse_texture, linear_repeat_sampler, varying.uv);

        float3 rgb = color.rgb;
        float3 hsv = rgb_to_hsv(rgb);
        color.rgb = multiply_rgb(rgb, rgb, saturate(hsv.z + 0.5));

        // if (material.environment_blend_mode != 0)
        // {
        //     float3 normal_vs = normalize(varying.normal_vs);

        //     float2 environment_uv = float2(normal_vs.x * 0.5 + 0.5, 1.0 - (normal_vs.y * 0.5 + 0.5));
        //     float4 environment_color = environment_texture.Sample(linear_clamp_sampler, environment_uv);

        //     if (material.environment_blend_mode == 1)
        //     {
        //         color *= float4(environment_color.rgb, 1.0);
        //     }
        //     else if (material.environment_blend_mode == 2)
        //     {
        //         color += float4(environment_color.rgb, 0.0);
        //     }
        // }

        float3 N = normalize(varying.normal_ws);

        float3 toon = 0.0;
        if (context.scene.light_count > 0)
        {
            StructuredBuffer<light_data> lights = ResourceDescriptorHeap[context.scene.light_buffer];
            light_data light = lights[0];

            float3 L = -light.direction;
            float NdotL = saturate(dot(N, L));

            if (toon_texture_id != 0)
            {
                Texture2D<float4> toon_texture = ResourceDescriptorHeap[toon_texture_id];
                toon = context.sample_texture(toon_texture, linear_clamp_sampler, float2(0.0, 1.0 - NdotL), 0.0).rgb * light.color;
            }
            else
            {
                toon = 1.0;
            }
            toon = float3(0.988,0.393,0.282);

            float3 diffuse_color = 1.0;
            float3 diffuse_brdf = diffuse_color / PI * NdotL * light.color;

            Texture2D<float3> ramp_texture = ResourceDescriptorHeap[ramp_texture_id];
            float position = diffuse_brdf.r * 0.2126 + diffuse_brdf.g * 0.7152 + diffuse_brdf.b * 0.0722;
            float3 ramp_color = context.sample_texture(ramp_texture, linear_clamp_sampler, float2(position, 0.0), 0.0);
            color.rgb = multiply_rgb(color.rgb, screen_rgb(ramp_color, toon, 1.0), 0.5);
        }
        // color.rgb = multiply_rgb(color.rgb, toon, saturate((1.0 - hsv.z) * hsv.z + 0.6));

        surface surface;
        surface.albedo = color.rgb;
        surface.roughness = 0.0;
        surface.metallic = 0.0;
        surface.emissive = 0.0;
        surface.normal_ws = N;

        return surface;
    }
};