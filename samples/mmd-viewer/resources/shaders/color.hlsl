#include "violet_common.hlsli"

struct vs_in
{
    vec3f position: POSITION;
    vec3f normal: NORMAL;
    vec2f uv: UV;
};

struct vs_out
{
    vec4f position : SV_POSITION;
    vec3f world_normal: WORLD_NORMAL;
    vec3f screen_normal: SCEEN_NORMAL;
    vec2f uv: UV;
};

ConstantBuffer<violet_mesh> mesh : register(b0, space0);

struct mmd_material
{
    vec4f diffuse;
    vec3f specular;
    float specular_strength;
    vec3f ambient;
    uint toon_mode;
    uint spa_mode;
};

ConstantBuffer<mmd_material> material : register(b0, space1);

[[vk::combinedImageSampler]]
Texture2D<vec4f> tex : register(t1, space1);
[[vk::combinedImageSampler]]
SamplerState tex_sampler : register(s1, space1);

[[vk::combinedImageSampler]]
Texture2D<vec4f> toon : register(t2, space1);
[[vk::combinedImageSampler]]
SamplerState toon_sampler : register(s2, space1);

[[vk::combinedImageSampler]]
Texture2D<vec4f> spa : register(t3, space1);
[[vk::combinedImageSampler]]
SamplerState spa_sampler : register(s3, space1);

ConstantBuffer<violet_camera> camera : register(b0, space2);
ConstantBuffer<violet_light> light : register(b0, space3);

vs_out vs_main(vs_in input)
{
    vs_out output;
    output.position = mul(camera.view_projection, mul(mesh.model, vec4f(input.position, 1.0)));
    output.world_normal = mul(input.normal, (vec3fx3)mesh.model);
    output.screen_normal = mul(output.world_normal, (vec3fx3)camera.view);
    output.uv = input.uv;

    return output;
}

vec4f fs_main(vs_out input) : SV_TARGET
{
    vec4f color = tex.Sample(tex_sampler, input. uv);
    vec3f world_normal = normalize(input.world_normal);
    vec3f screen_normal = normalize(input.screen_normal);
    vec4f light_color = vec4f(light.directional_lights[0].color, 1.0);
    color = color * material.diffuse * light_color;

    if (material.spa_mode != 0)
    {
        vec2f spa_uv = vec2f(screen_normal.x * 0.5 + 0.5, 1.0 - (screen_normal.y * 0.5 + 0.5));
        vec4f spa_color = spa.Sample(spa_sampler, spa_uv);

        if (material.spa_mode == 1) {
            color *= vec4f(spa_color.rgb, 1.0);
        }
        else if (material.spa_mode == 2)
            color += vec4f(spa_color.rgb, 0.0);
    }

    vec3f light_direction = normalize(light.directional_lights[0].direction);
    if (material.toon_mode != 0)
    {
        float c = 0.0;
        c = max(0.0, dot(world_normal, -light_direction));
        // c *= shadow_factor(i, pin.camera_depth, pin.shadow_position[i]);
        c = 1.0 - c;

        color *= toon.Sample(toon_sampler, vec2f(0.0, c));
    }

    return color;
}