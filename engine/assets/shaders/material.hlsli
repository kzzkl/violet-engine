#ifndef MATERIAL_HLSL
#define MATERIAL_HLSL

#include "surface.hlsli"
#include "mesh.hlsli"

struct material_context
{
#if !USE_RASTER_INTERPOLATION
    float2 ddx;
    float2 ddy;
#endif

    template <typename T>
    T sample_texture(Texture2D<T> texture, SamplerState sampler, float2 uv)
    {
#if USE_RASTER_INTERPOLATION
        return texture.Sample(sampler, uv);
#else
        return texture.SampleGrad(sampler, uv, ddx, ddy);
#endif
    }
};

struct material_common
{
    uint4 data;

    uint get_shading_model()
    {
        return data.x & 0x000000FF;
    }

    uint get_shadow_batch()
    {
        uint batch = get_opacity_cutoff() == 0 ? 0 : 1;
        return batch | (get_shadow_cull_mode() << 1);
    }

    uint get_shadow_cull_mode()
    {
        return (data.x >> 8) & 0x0000000F;
    }

    uint get_opacity_cutoff()
    {
        return data.y & 0x000000FF;
    }

    uint get_opacity_mask()
    {
        return (data.y >> 8) & 0x00FFFFFF;
    }

    uint get_resolve_pipeline()
    {
        return data.z;
    }
};

template <typename T>
T load_material(uint material_buffer, uint material_address)
{
    ByteAddressBuffer buffer = ResourceDescriptorHeap[material_buffer];
    return buffer.Load<T>(material_address);
}

template <typename T>
T load_material_data(uint material_buffer, uint material_address)
{
    return load_material<T>(material_buffer, material_address + sizeof(material_common));
}

#endif