#include "common.hlsli"
#include "gbuffer.hlsli" 

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

struct vs_in
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float3 normal_ws : NORMAL_WS;
    float3 normal_vs : NORMAL_VS;
    float2 texcoord : TEXCOORD;
    uint material_address : MATERIAL_ADDRESS;
};

vs_out vs_main(vs_in input, uint instance_index : SV_InstanceID)
{
    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];

    instance_data instance = instances[instance_index];
    mesh_data mesh = meshes[instance.mesh_index];

    vs_out output;
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
};

gbuffer::packed fs_main(vs_out input)
{
    SamplerState linear_clamp_sampler = SamplerDescriptorHeap[scene.linear_clamp_sampler];
    
    mmd_material material = load_material<mmd_material>(scene.material_buffer, input.material_address);

    Texture2D<float4> diffuse_texture = ResourceDescriptorHeap[material.diffuse_texture];
    Texture2D<float4> environment_texture = ResourceDescriptorHeap[material.environment_texture];

    float4 color = material.diffuse * diffuse_texture.Sample(linear_clamp_sampler, input.texcoord);

    if (material.environment_blend_mode != 0)
    {
        float3 normal_vs = normalize(input.normal_vs);

        float2 environment_texcoord = float2(normal_vs.x * 0.5 + 0.5, 1.0 - (normal_vs.y * 0.5 + 0.5));
        float4 environment_color = environment_texture.Sample(linear_clamp_sampler, environment_texcoord);

        if (material.environment_blend_mode == 1)
        {
            color *= float4(environment_color.rgb, 1.0);
        }
        else if (material.environment_blend_mode == 2)
        {
            color += float4(environment_color.rgb, 0.0);
        }
    }

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
            color.rgb *= toon_texture.Sample(linear_clamp_sampler, float2(0.0, 1.0 - NdotL)).rgb * light.color;
        }
        else
        {
            color.rgb *= light.color;
        }
    }

    gbuffer::data gbuffer_data;
    gbuffer_data.albedo = color.rgb;

    return gbuffer::pack(gbuffer_data);
}