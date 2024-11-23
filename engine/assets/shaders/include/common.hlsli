#ifndef COMMON_HLSLI
#define COMMON_HLSLI

static const float PI = 3.141592654;
static const float TWO_PI = 2.0 * PI;
static const float HALF_PI = 0.5 * PI;

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

struct light
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
    light lights[32];
    uint light_count;

    uint mesh_count;
    uint instance_count;
    uint padding0;
};

struct camera_data
{
    float4x4 view;
    float4x4 project;
    float4x4 view_projection;
    float3 position;
    uint padding;
};

#endif