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

static const uint RENDER_MESH_FRUSTUM_CULLING = 1 << 0;
static const uint RENDER_MESH_OCCLUSION_CULLING = 1 << 1;

struct mesh_data
{
    float4x4 model_matrix;
    float3 aabb_min;
    uint flags;
    float3 aabb_max;
    uint index_offset;
    uint position_address;
    uint normal_address;
    uint tangent_address;
    uint texcoord_address;
    uint4 custom_addresses;
};

struct instance_data
{
    uint mesh_index;
    uint vertex_offset;
    uint index_offset;
    uint index_count;

    uint batch_index;
    uint material_address;
    uint flags;
    uint padding_0;
};

static const uint LIGHT_DIRECTIONAL = 0;

struct light_data
{
    float3 position;
    uint type;
    float3 direction;
    bool shadow;
    float3 color;
    uint padding_0;
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
    uint vertex_buffer;
    uint index_buffer;
    uint skybox;
    uint irradiance;
    uint prefilter;
    uint padding_0;
    uint padding_1;
    uint padding_2;
};

struct camera_data
{
    float4x4 view;
    float4x4 projection;
    float4x4 projection_inv;
    float4x4 view_projection;
    float4x4 view_projection_inv;
    float4x4 view_projection_no_jitter;

    float4x4 prev_view_projection;
    float4x4 prev_view_projection_no_jitter;

    float3 position;
    float fov;

    float2 jitter;

    uint padding_0;
    uint padding_1;
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
        buffer[vertex_index * 3 + 0],
        buffer[vertex_index * 3 + 1],
        buffer[vertex_index * 3 + 2]);
    morph *= 0.0001; // morph precision

    return morph;
}

float4 reconstruct_position(float depth, float2 texcoord, float4x4 matrix_inv)
{
    texcoord.y = 1.0 - texcoord.y;
    float4 position_cs = float4(texcoord * 2.0 - 1.0, depth, 1.0);
    float4 position_ws = mul(matrix_inv, position_cs);

    return position_ws / position_ws.w;
}

float4 reconstruct_position(uint depth_buffer, float2 texcoord, float4x4 matrix_inv)
{
    Texture2D<float> buffer = ResourceDescriptorHeap[depth_buffer];
    float depth = buffer.SampleLevel(get_point_clamp_sampler(), texcoord, 0.0);
    return reconstruct_position(depth, texcoord, matrix_inv);
}

float2 normal_to_octahedron(float3 N)
{
    N.xy /= dot(1, abs(N));
    if (N.z <= 0)
    {
        N.xy = (1 - abs(N.yx)) * select(N.xy >= 0, float2(1, 1), float2(-1, -1));
    }
    N.xy = N.xy * 0.5 + 0.5;
    return N.xy;
}

float3 octahedron_to_normal(float2 oct)
{
    oct = oct * 2.0 - 1.0;
    float3 N = float3(oct, 1 - dot(1, abs(oct)));
    if (N.z < 0)
    {
        N.xy = (1 - abs(N.yx)) * select(N.xy >= 0, float2(1, 1), float2(-1, -1));
    }
    return normalize(N);
}

float luminance(float3 color)
{
    return 0.25 * color.r + 0.5 * color.g + 0.25 * color.b;
}

float3 tonemap(float3 color)
{
    return color / (1 + luminance(color));
}

float3 tonemap_invert(float3 color)
{
    return color / (1 - luminance(color));
}

float2 get_compute_texcoord(uint2 pixel_coord, uint width, uint height)
{
    return (float2(pixel_coord) + 0.5) / float2(width, height);
}

#endif