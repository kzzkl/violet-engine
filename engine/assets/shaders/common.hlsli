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

static const uint GBUFFER_ALBEDO = 0;
static const uint GBUFFER_MATERIAL = 1;
static const uint GBUFFER_NORMAL = 2;
static const uint GBUFFER_EMISSIVE = 3;

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

struct geometry_data
{
    float4 bounding_sphere;
    uint position_address;
    uint normal_address;
    uint tangent_address;
    uint texcoord_address;
    uint4 custom_addresses;
    uint index_offset;
    uint index_count;
    uint cluster_root;
    uint padding0;
};

struct mesh_data
{
    float4x4 matrix_m;
    float4 scale;
    float4x4 prev_matrix_m;
};

struct instance_data
{
    uint mesh_index;
    uint geometry_index;
    uint batch_index;
    uint material_address;
};

struct cluster_data
{
    float4 bounding_sphere;
    float4 lod_bounds;
    float lod_error;
    uint index_offset;
    uint index_count;
    uint padding0;
};

static const uint LIGHT_DIRECTIONAL = 0;

struct light_data
{
    float3 position;
    uint type;
    float3 direction;
    uint vsm_address;
    float3 color;
    uint padding0;
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
    uint skybox;
    uint irradiance;
    uint prefilter;
    uint padding0;
};

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
    float fov;

    float near;
    float far;
    float width;
    float height;

    float2 jitter;
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

struct material_info
{
    uint material_index;
    uint shading_model;
};

material_info load_material_info(uint material_buffer, uint material_address)
{
    ByteAddressBuffer buffer = ResourceDescriptorHeap[material_buffer];
    uint pack = buffer.Load<uint>(material_address);

    material_info info;
    info.material_index = (pack & 0xFFFFFF00) >> 8;
    info.shading_model = pack & 0xFF;
    return info;
}

template <typename T>
T load_material(uint material_buffer, uint material_address)
{
    ByteAddressBuffer buffer = ResourceDescriptorHeap[material_buffer];
    return buffer.Load<T>(material_address + sizeof(uint)); // Skip material info.
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

float2 get_compute_texcoord(uint2 texel_coord, uint width, uint height)
{
    return (float2(texel_coord) + 0.5) / float2(width, height);
}

#endif