#version 450

layout(location = 0) in vec3 vs_in_position;
layout(location = 1) in vec3 vs_in_normal;
layout(location = 2) in vec2 vs_in_uv;
layout(location = 3) in uvec4 vs_in_bine;
layout(location = 4) in vec3 vs_in_weight;

layout(location = 0) out vec4 vs_out_position;
layout(location = 1) out vec3 vs_out_normal;
layout(location = 2) out vec2 vs_out_uv;

layout(set = 0, binding = 0) uniform ash_object_t {
    mat4 transform_m;
    mat4 transform_mv;
    mat4 transform_mvp;
} ash_object;

layout(set = 2, binding = 0) uniform mmd_skeleton_t {
    mat4 offset[512];
} mmd_skeleton;

layout(set = 3, binding = 0) uniform ash_pass_t {
    vec4 camera_position;
    vec4 camera_direction;

    mat4 transform_v;
    mat4 transform_p;
    mat4 transform_vp;
} ash_pass;

void main()
{
    float weights[4] = { 0.0, 0.0, 0.0, 0.0 };
    weights[0] = vs_in_weight.x;
    weights[1] = vs_in_weight.y;
    weights[2] = vs_in_weight.z;
    weights[3] = 1.0f - weights[0] - weights[1] - weights[2];

    mat4 m = mat4(0.0f);
    for (int i = 0; i < 4; ++i)
        m += weights[i] * mmd_skeleton.offset[vs_in_bine[i]];

    mat4 mvp = m * ash_pass.transform_vp;
    mat4 mv = m * ash_pass.transform_v;

    vs_out_position = vec4(vs_in_position, 1.0f) * mvp;
    vs_out_normal = (vec4(vs_in_normal, 0.0f) * mv).xyz;
    vs_out_uv = vs_in_uv;

    gl_Position = vs_out_position;
}