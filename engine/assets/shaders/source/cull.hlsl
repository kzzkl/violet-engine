#include "common.hlsli"

ConstantBuffer<scene_data> scene : register(b0, space1);

struct cull_data
{
    uint cull_result;
    uint padding0;
    uint padding1;
    uint padding2;
};
ConstantBuffer<cull_data> cull : register(b0, space3);

[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    uint mesh_index = dtid.x;
    if (mesh_index >= scene.mesh_count)
    {
        return;
    }

    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];

    mesh_data mesh = meshes[mesh_index];

    RWByteAddressBuffer cull_result = ResourceDescriptorHeap[cull.cull_result];
    cull_result.Store(mesh_index * 4, 1);
}