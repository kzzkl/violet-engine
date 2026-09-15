#ifndef COMMON_HLSLI
#define COMMON_HLSLI

#if defined(__spirv__)
#define PushConstant(constant_type, constant_name) [[vk::push_constant]] constant_type constant_name
#else
#define PushConstant(constant_type, constant_name) ConstantBuffer<constant_type> constant_name : register(b100)
#endif

static const float PI = 3.141592654;
static const float TWO_PI = 2.0 * PI;
static const float HALF_PI = 0.5 * PI;

static const float EPSILON = 1e-10;

static const uint SHADING_TILE_SIZE = 16;

struct draw_command
{
    uint index_count;
    uint instance_count;
    uint index_offset;
    uint vertex_offset;
    uint instance_offset;
};

struct draw_info
{
    uint instance_id;
    uint cluster_id;
};

struct dispatch_command
{
    uint x;
    uint y;
    uint z;
};

enum geometry_attribute
{
    GEOMETRY_ATTRIBUTE_POSITION = 0,
    GEOMETRY_ATTRIBUTE_NORMAL = 1,
    GEOMETRY_ATTRIBUTE_TANGENT = 2,
    GEOMETRY_ATTRIBUTE_UV = 3,
    GEOMETRY_ATTRIBUTE_CUSTOM0 = 4,
    GEOMETRY_ATTRIBUTE_CUSTOM1 = 5,
    GEOMETRY_ATTRIBUTE_CUSTOM2 = 6,
    GEOMETRY_ATTRIBUTE_CUSTOM3 = 7,
};

struct geometry_data
{
    uint attributes[8];
    uint index_offset;
    uint index_count;
    uint cluster_root;
    uint padding0;
    float3 bounding_box_min;
    uint padding1;
    float3 bounding_box_max;
    uint padding2;
    float4 bounding_sphere;
};

static const uint MESH_STATIC = 1 << 0;
static const uint MESH_SKIP_FRUSTUM_CULL = 1 << 1;
static const uint MESH_SKIP_OCCLUSION_CULL = 1 << 2;

struct mesh_data
{
    float4x4 matrix_m;
    float4 scale;
    float4x4 prev_matrix_m;

    uint flags;
    uint padding0;
    uint padding1;
    uint padding2;
};

struct instance_data
{
    uint mesh_id;
    uint submesh_id;
    uint batch_index;
    uint material_address;
};

static const uint LIGHT_DIRECTIONAL = 0;

struct light_data
{
    float3 position;
    uint type;
    float3 direction;
    uint cast_shadow;
    float3 color;
    uint padding;
};

struct scene_data
{
    uint mesh_buffer;
    uint mesh_count;
    uint instance_buffer;
    uint instance_count;
    uint light_buffer;
    uint light_count;
    uint batch_buffer;
    uint material_buffer;
    uint geometry_buffer;
    uint cluster_buffer;
    uint vertex_buffer;
    uint index_buffer;
};

static const uint CAMERA_PERSPECTIVE = 0;
static const uint CAMERA_ORTHOGRAPHIC = 1;

struct camera_data
{
    float4x4 matrix_v;
    float4x4 matrix_p;
    float4x4 matrix_p_inv;
    float4x4 matrix_vp;
    float4x4 matrix_vp_inv;
    float4x4 matrix_vp_no_jitter;

    float4x4 prev_matrix_v;
    float4x4 prev_matrix_p;
    float4x4 prev_matrix_vp;
    float4x4 prev_matrix_vp_no_jitter;

    float3 position;
    uint camera_id;

    float2 jitter;

    float near;
    float far;

    float aspect;
    uint type;
    float perspective_fov;
    float orthographic_size;

    float4 frustum; // perspective frustum

    float texel_size;
    float texel_size_inv;
    uint padding0;
    uint padding1;
};

SamplerState get_point_repeat_sampler()
{
    return SamplerDescriptorHeap[0];
}

SamplerState get_point_mirrored_repeat_sampler()
{
    return SamplerDescriptorHeap[1];
}

SamplerState get_point_clamp_sampler()
{
    return SamplerDescriptorHeap[2];
}

SamplerState get_linear_repeat_sampler()
{
    return SamplerDescriptorHeap[3];
}

SamplerState get_linear_mirrored_repeat_sampler()
{
    return SamplerDescriptorHeap[4];
}

SamplerState get_linear_clamp_sampler()
{
    return SamplerDescriptorHeap[5];
}

float3 get_morph_position(uint morph_vertex_buffer, uint vertex_index)
{
    StructuredBuffer<int> buffer = ResourceDescriptorHeap[morph_vertex_buffer];

    float3 morph = float3(
        buffer[vertex_index * 3 + 0],
        buffer[vertex_index * 3 + 1],
        buffer[vertex_index * 3 + 2]);
    morph *= 0.0001; // Morph precision

    return morph;
}

float4 reconstruct_position(float depth, float2 uv, float4x4 matrix_inv)
{
    uv.y = 1.0 - uv.y;
    float4 position_cs = float4(uv * 2.0 - 1.0, depth, 1.0);
    float4 position_ws = mul(matrix_inv, position_cs);

    return position_ws / position_ws.w;
}

float4 reconstruct_position(uint depth_buffer, float2 uv, float4x4 matrix_inv)
{
    Texture2D<float> buffer = ResourceDescriptorHeap[depth_buffer];
    float depth = buffer.SampleLevel(get_point_clamp_sampler(), uv, 0.0);
    return reconstruct_position(depth, uv, matrix_inv);
}

float get_luminance(float3 color)
{
    return dot(color, float3(0.2126729, 0.7151522, 0.0721750));
}

float3 tonemap(float3 color)
{
    return color / (1 + get_luminance(color));
}

float3 tonemap_invert(float3 color)
{
    return color / (1 - get_luminance(color));
}

float2 get_compute_uv(uint2 texel_coord, uint width, uint height)
{
    return (float2(texel_coord) + 0.5) / float2(width, height);
}

uint get_dispatch_group_count(uint offset, uint count, uint group_size)
{
    return (offset + count + group_size - 1) / group_size - (offset + group_size - 1) / group_size;
}

#endif