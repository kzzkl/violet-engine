#include "atmosphere/atmosphere.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    atmosphere_data atmosphere;
    float3 sun_direction;
    float distance_per_slice;
    float3 sun_irradiance;
    uint sample_count;
    float3 frustum_top_left;
    uint transmittance_lut;
    float3 frustum_top_right;
    uint aerial_perspective_lut;
    float3 frustum_bottom_left;
    uint multi_scattering_lut;
    float3 frustum_bottom_right;
    uint vsm_id;
    uint vsm_buffer;
    uint vsm_virtual_page_table;
    uint vsm_physical_shadow_map;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture3D<float4> aerial_perspective_lut = ResourceDescriptorHeap[constant.aerial_perspective_lut];

    uint width;
    uint height;
    uint depth;
    aerial_perspective_lut.GetDimensions(width, height, depth);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }

    atmosphere_data atmosphere = constant.atmosphere;

    Texture2D<float3> transmittance_lut = ResourceDescriptorHeap[constant.transmittance_lut];
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    float3 eye = camera.position + float3(0.0, atmosphere.planet_radius, 0.0);
    eye.y = max(eye.y, atmosphere.planet_radius + 1.0);

    float2 uv = get_compute_texcoord(dtid.xy, width, height);
    float3 view = normalize(lerp(
        lerp(constant.frustum_top_left, constant.frustum_top_right, uv.x),
        lerp(constant.frustum_bottom_left, constant.frustum_bottom_right, uv.x), uv.y));

    if (!move_to_atmosphere(eye, view, atmosphere.atmosphere_radius))
    {
        for (uint slice_id = 0; slice_id < depth; ++slice_id)
        {
            aerial_perspective_lut[uint3(dtid.x, dtid.y, slice_id)] = 0.0;
        }
        return;
    }

    float distance = ray_sphere_intersection(eye, view, 0.0, atmosphere.planet_radius);
    if (distance < 0.0)
    {
        distance = ray_sphere_intersection(eye, view, 0.0, atmosphere.atmosphere_radius);
    }

#ifdef USE_MULTI_SCATTERING
    Texture2D<float3> multi_scattering_lut = ResourceDescriptorHeap[constant.multi_scattering_lut];
#endif

#ifdef USE_SHADOW
    StructuredBuffer<vsm_data> vsms = ResourceDescriptorHeap[constant.vsm_buffer];
    StructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];
    Texture2D<uint> physical_shadow_map = ResourceDescriptorHeap[constant.vsm_physical_shadow_map];
#endif

    uint slice_id = dtid.z;

    float slice = (float(slice_id) + 0.5) / depth;
    slice *= slice;
    slice *= depth;

    float slice_end = min(distance, slice * constant.distance_per_slice);
    if (slice_end <= 0.0)
    {
        aerial_perspective_lut[uint3(dtid.x, dtid.y, slice_id)] = 0.0;
    }
    else
    {
        float4 result = integrate_atmosphere(
            atmosphere,
            eye,
            view,
            constant.sun_direction,
            slice_end,
            constant.sample_count,
            transmittance_lut
#ifdef USE_MULTI_SCATTERING
            ,
            multi_scattering_lut
#endif
#ifdef USE_SHADOW
            ,
            constant.vsm_id,
            vsms,
            virtual_page_table,
            physical_shadow_map
#endif
        );

        result.xyz *= constant.sun_irradiance;
        aerial_perspective_lut[uint3(dtid.x, dtid.y, slice_id)] = result;
    }
}