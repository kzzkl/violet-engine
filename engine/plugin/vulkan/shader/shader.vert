#version 450

layout(location = 0) in vec2 vs_in_position;
layout(location = 1) in vec3 vs_in_color;
layout(location = 2) in vec2 vs_in_uv;

layout(location = 0) out vec3 vs_out_color;
layout(location = 1) out vec2 vs_out_uv;

layout(binding = 0) uniform UniformBufferObject {
    vec3 color;
} ubo;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = vec4(vs_in_position, 0.0, 1.0);
    vs_out_color = ubo.color;
    vs_out_uv = vs_in_uv;
}