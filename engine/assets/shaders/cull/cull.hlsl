#include "common.hlsli"

ConstantBuffer<scene_data> scene : register(b0, space1);

struct constant_data
{
    uint cull_result;
    uint padding_0;
    uint padding_1;
    uint padding_2;
};
PushConstant(constant_data, constant);

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

    RWByteAddressBuffer cull_result = ResourceDescriptorHeap[constant.cull_result];
    cull_result.Store(mesh_index * 4, 1);
}