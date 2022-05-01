#version 450

layout(location = 0) in vec3 vs_out_color;
layout(location = 1) in vec2 vs_out_uv;

layout(location = 0) out vec4 ps_out_color;

layout(binding = 1) uniform sampler2D texture_sampler;

void main() {
    // ps_out_color = vec4(vs_out_color * texture(texture_sampler, vs_out_uv).rgb, 1.0f);
    ps_out_color = vec4(vs_out_uv, 0.0, 1.0);
}