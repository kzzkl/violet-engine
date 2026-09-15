#include "common.hlsli"
#include "distance_field/sdf_common.hlsli"

struct constant_data
{
    uint clipmap_state;
    uint clipmap_level_mesh_counts;

    uint invalidated_grid_indirect_args;
    uint invalidated_page_indirect_args;

    uint pages_to_allocate_indirect_args;
};
PushConstant(constant_data, constant);

[shader("compute")]
[numthreads(1, 1, 1)]
void cs_main()
{
    RWStructuredBuffer<uint> clipmap_level_mesh_counts = ResourceDescriptorHeap[constant.clipmap_level_mesh_counts];

    for (uint level = 0; level < SDF_CLIPMAP_LEVEL_COUNT; ++level)
    {
        clipmap_level_mesh_counts[level] = 0;
    }

    RWStructuredBuffer<clipmap_state> clipmap_state = ResourceDescriptorHeap[constant.clipmap_state];
    clipmap_state[0].invalidated_grid_count = 0;
    clipmap_state[0].invalidated_grid_mesh_count = 0;
    clipmap_state[0].invalidated_page_count = 0;
    clipmap_state[0].pages_to_allocate_count = 0;
    clipmap_state[0].pages_to_update_count = 0;
    clipmap_state[0].free_page_atlas_count = max(clipmap_state[0].free_page_atlas_count, 0);

    dispatch_command command;
    command.x = 0;
    command.y = 1;
    command.z = 1;

    RWStructuredBuffer<dispatch_command> invalidated_grid_indirect_args = ResourceDescriptorHeap[constant.invalidated_grid_indirect_args];
    invalidated_grid_indirect_args[0] = command;

    RWStructuredBuffer<dispatch_command> invalidated_page_indirect_args = ResourceDescriptorHeap[constant.invalidated_page_indirect_args];
    invalidated_page_indirect_args[0] = command;

    RWStructuredBuffer<dispatch_command> pages_to_allocate_indirect_args = ResourceDescriptorHeap[constant.pages_to_allocate_indirect_args];
    pages_to_allocate_indirect_args[0] = command;
}