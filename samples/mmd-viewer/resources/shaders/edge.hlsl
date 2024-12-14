#include "violet_common.hlsli"

ConstantBuffer<violet_mesh> mesh : register(b0, space0);

struct mmd_material
{
    vec4f edge_color;
    float edge_size;
};

ConstantBuffer<mmd_material> material : register(b0, space1);
ConstantBuffer<violet_camera> camera : register(b0, space2);

struct vs_in
{
    vec3f position : POSITION;
    vec3f normal : NORMAL;
    float edge : EDGE;
};

struct vs_out
{
    vec4f position : SV_POSITION;
};

vs_out vs_main(vs_in input)
{
    vec4f position = mul(camera.view_projection, mul(mesh.model, vec4f(input.position, 1.0)));
    vec3f normal = mul((vec3fx3)camera.view, mul((vec3fx3)mesh.model, input.normal));
    vec2f screen_normal = normalize(normal.xy);

    position.xy += screen_normal * 0.003f * material.edge_size * input.edge * position.w;

    vs_out output;
    output.position = position;
    return output;
}

vec4f fs_main(vs_out input) : SV_TARGET
{
    return material.edge_color;
}