#include "common.hlsli"

RWByteAddressBuffer cull_result : register(u0, space0);

ConstantBuffer<scene_data> scene : register(b0, space1);
StructuredBuffer<mesh_data> meshes : register(t1, space1);

[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    uint mesh_index = dtid.x;
    if (mesh_index >= scene.mesh_count)
    {
        return;
    }

    mesh_data mesh = meshes[mesh_index];

    cull_result.Store(mesh_index * 4, 1);
}