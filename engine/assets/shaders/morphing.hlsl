#include "common.hlsli"

struct constant_data
{
    uint morph_target_count;
    float precision;
    uint header_buffer;
    uint element_buffer;
    uint morph_vertex_buffer;
    uint weight_buffer;
    uint padding0;
    uint padding1;
};
PushConstant(constant_data, constant);

struct morph_target_header
{
    uint element_count;
    uint element_offset;
    int position_min;
    uint padding0;
};

struct morph_element
{
    int3 position;
    uint vertex_index;
};

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    uint morph_target_index = dtid.x;
    uint morph_element_index = dtid.y;

    if (morph_target_index >= constant.morph_target_count)
    {
        return;
    }

    StructuredBuffer<morph_target_header> header_buffer = ResourceDescriptorHeap[constant.header_buffer];
    StructuredBuffer<morph_element> element_buffer = ResourceDescriptorHeap[constant.element_buffer];

    StructuredBuffer<float4> weight_buffer = ResourceDescriptorHeap[constant.weight_buffer];
    RWStructuredBuffer<int> morph_vertex_buffer = ResourceDescriptorHeap[constant.morph_vertex_buffer];

    morph_target_header header = header_buffer[morph_target_index];

    if (morph_element_index >= header.element_count)
    {
        return;
    }

    morph_element element = element_buffer[header.element_offset + morph_element_index];

    float3 position = (float3)(element.position + header.position_min) * constant.precision;
    position *= weight_buffer[morph_target_index >> 2][morph_target_index & 3];

    int3 p = (int3)(element.position + header.position_min);

    int3 quantized_position = (int3)round(position / constant.precision);

    uint vertex_index = element.vertex_index * 3;
    InterlockedAdd(morph_vertex_buffer[vertex_index + 0], quantized_position.x);
    InterlockedAdd(morph_vertex_buffer[vertex_index + 1], quantized_position.y);
    InterlockedAdd(morph_vertex_buffer[vertex_index + 2], quantized_position.z);
}