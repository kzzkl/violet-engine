#ifndef COMMON_HLSLI
#define COMMON_HLSLI

static const float PI = 3.141592654;
static const float TWO_PI = 2.0 * PI;
static const float HALF_PI = 0.5 * PI;

static const float EPSILON = 1e-5;

static const uint RENDER_MESH_FRUSTUM_CULLING = 1 << 0;
static const uint RENDER_MESH_OCCLUSION_CULLING = 1 << 1;

struct mesh_data
{
    float4x4 model_matrix;
    float3 aabb_min;
    uint flags;
    float3 aabb_max;
    uint padding0;
};

struct instance_data
{
    uint mesh_index;
    uint vertex_offset;
    uint index_offset;
    uint index_count;

    uint group_index;
    uint material_address;
    uint flags;
    uint padding0;
};

static const uint LIGHT_DIRECTIONAL = 0;

struct light_data
{
    float3 position;
    uint type;
    float3 direction;
    bool shadow;
    float3 color;
    uint padding0;
};

struct scene_data
{
    uint mesh_buffer;
    uint mesh_count;
    uint instance_buffer;
    uint instance_count;
    uint group_buffer;
    uint light_buffer;
    uint light_count;
    uint skybox;
    uint irradiance;
    uint prefilter;
    uint brdf_lut;
    uint material_buffer;
    uint point_repeat_sampler;
    uint point_clamp_sampler;
    uint linear_repeat_sampler;
    uint linear_clamp_sampler;
};

struct camera_data
{
    float4x4 view;
    float4x4 project;
    float4x4 view_projection;
    float4x4 view_projection_inv;
    float3 position;
    float fov;
};

template <typename T>
T load_material(uint material_buffer, uint material_address)
{
    ByteAddressBuffer buffer = ResourceDescriptorHeap[material_buffer];
    return buffer.Load<T>(material_address);
}

float3 get_morph_position(uint morph_vertex_buffer, uint vertex_index)
{
    Buffer<int> buffer = ResourceDescriptorHeap[morph_vertex_buffer];

    float3 morph = float3(
        buffer[vertex_index + 0],
        buffer[vertex_index + 1],
        buffer[vertex_index + 2]);
    morph *= 0.0001; // morph precision

    return morph;
}

#endif