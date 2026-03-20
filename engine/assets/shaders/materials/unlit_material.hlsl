#include "visibility/material_resolve.hlsli"

struct unlit_material
{
    float3 albedo;

    gbuffer resolve(vertex vertex, float2 ddx, float2 ddy)
    {
        gbuffer gbuffer;
        gbuffer.albedo = albedo;
        gbuffer.roughness = 0.0;
        gbuffer.metallic = 0.0;
        gbuffer.emissive = 0.0;
        gbuffer.normal = vertex.normal;
        return gbuffer;
    }
};

[numthreads(8, 8, 1)]
void cs_main(uint3 gtid : SV_GroupThreadID, uint3 gid : SV_GroupID)
{
    resolve_material<unlit_material>(gtid, gid);
}