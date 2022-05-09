#version 450

layout(location = 0) in vec4 ps_in_position;
layout(location = 1) in vec3 ps_in_normal;
layout(location = 2) in vec2 ps_in_uv;

layout(set = 1, binding = 0) uniform mmd_material_t {
    vec4 diffuse;
    vec3 specular;
    float specular_strength;
    uint toon_mode;
    uint spa_mode;
} mmd_material;

layout(set = 1, binding = 1) uniform sampler2D tex_sampler;
layout(set = 1, binding = 2) uniform sampler2D toon_sampler;
layout(set = 1, binding = 3) uniform sampler2D spa_sampler;

layout(location = 0) out vec4 ps_out_color;

void main()
{
    vec4 color = mmd_material.diffuse;
    color = color * texture(tex_sampler, ps_in_uv);

    vec3 normal = normalize(ps_in_normal);
    if (mmd_material.spa_mode != 0)
    {
        vec2 spa_uv = {normal.x * 0.5f + 0.5f, 1.0f - (normal.y * 0.5f + 0.5f)};
        vec3 spa_color = texture(spa_sampler, spa_uv).rgb;

        if (mmd_material.spa_mode == 1)
            color *= vec4(spa_color, 1.0f);
        else if (mmd_material.spa_mode == 2)
            color += vec4(spa_color, 0.0f);
    }

    if (mmd_material.toon_mode != 0)
    {
        vec3 light = {1.0f, 1.0f, 1.0f};
        vec3 light_dir = normalize(vec3(1.0f, -1.0f, 1.0f));

        float c = dot(normal, light_dir);
        c = clamp(c + 0.5f, 0.0f, 1.0f);
        
        color *= texture(toon_sampler, vec2(0.0f, c));
    }

    ps_out_color = color;
}