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
    float3 world_normal : WORLD_NORMAL;
    float3 screen_normal : SCEEN_NORMAL;
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
    output.world_normal = mul((float3x3)mesh.model_matrix, input.normal);
    output.screen_normal = mul((float3x3)camera.view, output.world_normal);
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
    float edge_size;
    float4 edge_color;
};

gbuffer::packed fs_main(vs_out input)
{
    SamplerState linear_repeat_sampler = SamplerDescriptorHeap[scene.linear_repeat_sampler];
    
    mmd_material material = load_material<mmd_material>(scene.material_buffer, input.material_address);

    Texture2D<float4> diffuse_texture = ResourceDescriptorHeap[material.diffuse_texture];
    Texture2D<float4> toon_texture = ResourceDescriptorHeap[material.toon_texture];
    Texture2D<float4> environment_texture = ResourceDescriptorHeap[material.environment_texture];

    float4 color = material.diffuse * diffuse_texture.Sample(linear_repeat_sampler, input.texcoord);

    if (material.environment_blend_mode != 0)
    {
        float3 screen_normal = normalize(input.screen_normal);

        float2 environment_texcoord = float2(screen_normal.x * 0.5 + 0.5, 1.0 - (screen_normal.y * 0.5 + 0.5));
        float4 environment_color = environment_texture.Sample(linear_repeat_sampler, environment_texcoord);

        if (material.environment_blend_mode == 1)
        {
            color *= float4(environment_color.rgb, 1.0);
        }
        else if (material.environment_blend_mode == 2)
        {
            color += float4(environment_color.rgb, 0.0);
        }
    }

    float3 L = normalize(float3(1.0, 1.0, 1.0));
    float3 N = normalize(input.world_normal);
    float c = saturate(dot(N, L));
    c = 1.0 - c;

    // color *= toon_texture.Sample(linear_repeat_sampler, float2(0.0, c));

    gbuffer::data gbuffer_data;
    gbuffer_data.albedo = color.rgb;
    gbuffer_data.roughness = 1.0;
    gbuffer_data.metallic = 0.0;
    gbuffer_data.normal = N;
    gbuffer_data.emissive = 0.0;

    return gbuffer::pack(gbuffer_data);
}