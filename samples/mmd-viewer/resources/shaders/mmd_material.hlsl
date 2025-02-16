#include "color.hlsli"
#include "brdf.hlsli"

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

struct vs_input
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
};

struct vs_output
{
    float4 position : SV_POSITION;
    float3 normal_ws : NORMAL_WS;
    float3 normal_vs : NORMAL_VS;
    float2 texcoord : TEXCOORD;
    uint material_address : MATERIAL_ADDRESS;
};

vs_output vs_main(vs_input input, uint instance_index : SV_InstanceID)
{
    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];

    instance_data instance = instances[instance_index];
    mesh_data mesh = meshes[instance.mesh_index];

    vs_output output;
    output.position = mul(camera.view_projection, mul(mesh.model_matrix, float4(input.position, 1.0)));
    output.normal_ws = mul((float3x3)mesh.model_matrix, input.normal);
    output.normal_vs = mul((float3x3)camera.view, output.normal_ws);
    output.texcoord = input.texcoord;
    output.material_address = instance.material_address;

    return output;
}

struct mmd_material
{
    float4 diffuse;
    float3 specular;
    float specular_strength;
    float3 ambient;
    uint diffuse_texture;
    uint toon_texture;
    uint environment_texture;
    uint environment_blend_mode;
    uint ramp_texture;
};

struct fs_output
{
    float4 color : SV_TARGET0;
    float2 normal : SV_TARGET1;
    float4 emissive : SV_TARGET2;
};

float3 multiply_rgb(float3 a, float3 b, float factor)
{
    return (1.0 - factor) * a + factor * a * b;
}

float3 screen_rgb(float3 a, float3 b, float factor)
{
    return (1.0 - factor) * a + factor * (1.0 - (1.0 - a) * (1.0 - b));
}

fs_output fs_main(vs_output input)
{
    SamplerState linear_repeat_sampler = get_linear_repeat_sampler();
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();
    
    mmd_material material = load_material<mmd_material>(scene.material_buffer, input.material_address);

    Texture2D<float4> diffuse_texture = ResourceDescriptorHeap[material.diffuse_texture];
    Texture2D<float4> environment_texture = ResourceDescriptorHeap[material.environment_texture];

    float4 color = material.diffuse * diffuse_texture.Sample(linear_repeat_sampler, input.texcoord);

    float3 rgb = color.rgb;
    float3 hsv = rgb_to_hsv(rgb);
    color.rgb = multiply_rgb(rgb, rgb, saturate(hsv.z + 0.5));

    // if (material.environment_blend_mode != 0)
    // {
    //     float3 normal_vs = normalize(input.normal_vs);

    //     float2 environment_texcoord = float2(normal_vs.x * 0.5 + 0.5, 1.0 - (normal_vs.y * 0.5 + 0.5));
    //     float4 environment_color = environment_texture.Sample(linear_clamp_sampler, environment_texcoord);

    //     if (material.environment_blend_mode == 1)
    //     {
    //         color *= float4(environment_color.rgb, 1.0);
    //     }
    //     else if (material.environment_blend_mode == 2)
    //     {
    //         color += float4(environment_color.rgb, 0.0);
    //     }
    // }

    float3 toon = 0.0;
    if (scene.light_count > 0)
    {
        StructuredBuffer<light_data> lights = ResourceDescriptorHeap[scene.light_buffer];
        light_data light = lights[0];

        float3 L = -light.direction;
        float3 N = normalize(input.normal_ws);
        float NdotL = saturate(dot(N, L));

        if (material.toon_texture != 0)
        {
            Texture2D<float4> toon_texture = ResourceDescriptorHeap[material.toon_texture];
            toon = toon_texture.Sample(linear_clamp_sampler, float2(0.0, 1.0 - NdotL)).rgb * light.color;
        }
        else
        {
            toon = 1.0;
        }
        toon = float3(0.988,0.393,0.282);
        
        float3 diffuse_color = 1.0;
        float3 diffuse_brdf = diffuse_color / PI * NdotL * light.color;

        Texture2D<float3> ramp_texture = ResourceDescriptorHeap[material.ramp_texture];
        float position = diffuse_brdf.r * 0.2126 + diffuse_brdf.g * 0.7152 + diffuse_brdf.b * 0.0722;
        float3 ramp_color = ramp_texture.Sample(linear_clamp_sampler, float2(position, 0.0)).rgb;
        color.rgb =multiply_rgb(color.rgb, screen_rgb(ramp_color, toon, 1.0), 0.5);
    }
    // color.rgb = multiply_rgb(color.rgb, toon, saturate((1.0 - hsv.z) * hsv.z + 0.6));

    fs_output output;
    output.color = color;
    output.normal = 0.0;
    output.emissive = 0.0;

    return output;
}