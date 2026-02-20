#include "virtual_shadow_map/vsm_common.hlsli"
#include "gbuffer.hlsli"

struct constant_data
{
    uint depth_buffer;
    uint normal_buffer;
    uint vsm_buffer;
    uint vsm_virtual_page_table;
    uint vsm_physical_shadow_map;
    uint vsm_directional_buffer;
    uint light_id;
    float normal_offset;
    float constant_bias;
    uint sample_mode;
    uint sample_count;
    float sample_radius;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

struct vs_output
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

static const uint PCF_SAMPLE_COUNT = 8;

float3 get_position_offset(float NdotL, float3 normal_ws, float normal_offset)
{
    return normal_offset * VIRTUAL_TEXEL_SIZE * saturate(1.0 - NdotL) * normal_ws;
}

float3 get_receiver_plane_bias(float3 position_ws, float4x4 cascade0_matrix_vp, uint cascade)
{
    float4 position_ls_cascade0 = mul(cascade0_matrix_vp, float4(position_ws, 1.0));
    position_ls_cascade0 /= position_ls_cascade0.w;
    position_ls_cascade0.xy = position_ls_cascade0.xy * 0.5 + 0.5;

    float3 dx = ddx_fine(position_ls_cascade0.xyz);
    float3 dy = ddy_fine(position_ls_cascade0.xyz);
    dx = ldexp(dx, cascade);
    dy = ldexp(dy, cascade);

    float2 bias_uv;
    bias_uv.x = dy.y * dx.z - dx.y * dy.z;
    bias_uv.y = dx.x * dy.z - dy.x * dx.z;
    bias_uv *= 1.0 / ((dx.x * dy.y) - (dx.y * dy.x));
    bias_uv = abs(bias_uv);

    return float3(bias_uv, min(dot(VIRTUAL_TEXEL_SIZE, bias_uv), 0.01));
}

bool sample_depth(uint vsm_id, float2 uv, Texture2D<uint> physical_shadow_map, StructuredBuffer<uint> virtual_page_table, out float depth)
{
    float2 virtual_page_coord_f = uv * VIRTUAL_PAGE_TABLE_SIZE;
    uint2 virtual_page_coord = floor(virtual_page_coord_f);
    float2 virtual_page_local_uv = frac(virtual_page_coord_f);

    uint virtual_page_index = get_virtual_page_index(vsm_id, virtual_page_coord);

    vsm_virtual_page virtual_page = vsm_virtual_page::unpack(virtual_page_table[virtual_page_index]);

    bool valid = virtual_page.flags & (VIRTUAL_PAGE_FLAG_CACHE_VALID | VIRTUAL_PAGE_FLAG_REQUEST);

    uint2 physical_texel = virtual_page.get_physical_texel(virtual_page_local_uv);
    depth = asfloat(physical_shadow_map[physical_texel]);

    return valid;
}

float sample_shadow(uint vsm_id, float3 position_ls, float3 bias, Texture2D<uint> physical_shadow_map, StructuredBuffer<uint> virtual_page_table)
{
    float shadow_depth;
    sample_depth(vsm_id, position_ls.xy, physical_shadow_map, virtual_page_table, shadow_depth);
    return shadow_depth > position_ls.z + bias.z ? 0.0 : 1.0;
}

float random(float2 uv)
{
    float a = 12.9898;
    float b = 78.233;
    float c = 43758.5453;
    float dt = dot(uv.xy, float2(a, b));
    float sn = fmod(dt, PI);
    return frac(sin(sn) * c);
}

float sample_shadow_pcf(uint vsm_id, float3 position_ls, float3 bias, float sample_radius, Texture2D<uint> physical_shadow_map, StructuredBuffer<uint> virtual_page_table)
{
    float2 poisson_disk[PCF_SAMPLE_COUNT];
    float angle_step = TWO_PI * float(10) / float(PCF_SAMPLE_COUNT);

    float angle = random(position_ls.xy) * TWO_PI;
    float radius = 1.0 / float(PCF_SAMPLE_COUNT);
    float radius_step = radius;

    for(int i = 0; i < PCF_SAMPLE_COUNT; i++)
    {
        poisson_disk[i] = float2(cos(angle), sin(angle)) * pow(radius, 0.75);
        radius += radius_step;
        angle += angle_step;
    }

    float visibility = 0.0;
    float valid_sample_count = 0.0;
    for(int i = 0; i < PCF_SAMPLE_COUNT; i++)
    {
        float2 offset = poisson_disk[i] * sample_radius;
        float sample_bias = bias.z + dot(abs(offset), bias.xy);
        float2 sample_uv = position_ls.xy + offset;

        float shadow_depth;

        if (sample_depth(vsm_id, sample_uv, physical_shadow_map, virtual_page_table, shadow_depth))
        {
            visibility += shadow_depth > position_ls.z + sample_bias ? 0.0 : 1.0;
            ++valid_sample_count;
        }
    }

    visibility /= valid_sample_count;

    return visibility;
}

float fs_main(vs_output input) : SV_TARGET
{
    Texture2D<float> depth_buffer = ResourceDescriptorHeap[constant.depth_buffer];
    float depth = depth_buffer.SampleLevel(get_point_clamp_sampler(), input.texcoord, 0.0);
    if (depth == 0.0)
    {
        return 0.0;
    }

    StructuredBuffer<light_data> lights = ResourceDescriptorHeap[scene.shadow_casting_light_buffer];
    light_data light = lights[constant.light_id];
    
    float3 position_ws = reconstruct_position(depth, input.texcoord, camera.matrix_vp_inv).xyz;

    Texture2D<uint> buffer = ResourceDescriptorHeap[constant.normal_buffer];
    uint width;
    uint height;
    buffer.GetDimensions(width, height);
    float3 normal_ws = unpack_gbuffer_normal(constant.normal_buffer, input.texcoord * float2(width, height));

    StructuredBuffer<uint> directional_vsms = ResourceDescriptorHeap[constant.vsm_directional_buffer];
    StructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];
    Texture2D<uint> physical_shadow_map = ResourceDescriptorHeap[constant.vsm_physical_shadow_map];

    uint vsm_id = get_directional_vsm_id(directional_vsms, light.vsm_address, camera.camera_id);
    uint cascade = get_directional_cascade(length(position_ws - camera.position));

    StructuredBuffer<vsm_data> vsms = ResourceDescriptorHeap[constant.vsm_buffer];
    vsm_data vsm = vsms[vsm_id + cascade];

    position_ws += get_position_offset(dot(normal_ws, light.direction), normal_ws, constant.normal_offset);

    float3 bias = get_receiver_plane_bias(position_ws, vsms[vsm_id].matrix_vp, cascade);
    bias.z += ldexp(constant.constant_bias * VIRTUAL_TEXEL_SIZE, -int(cascade));

    float4 position_ls = mul(vsm.matrix_vp, float4(position_ws, 1.0));
    position_ls /= position_ls.w;
    position_ls.xy = position_ls.xy * 0.5 + 0.5;

    if (constant.sample_mode == 1)
    {
        float sample_radius = constant.sample_radius * vsm.texel_size_inv * VIRTUAL_TEXEL_SIZE;
        return sample_shadow_pcf(vsm_id + cascade, position_ls.xyz, bias, sample_radius, physical_shadow_map, virtual_page_table);
    }
    else
    {
        return sample_shadow(vsm_id + cascade, position_ls.xyz, bias, physical_shadow_map, virtual_page_table);
    }
}