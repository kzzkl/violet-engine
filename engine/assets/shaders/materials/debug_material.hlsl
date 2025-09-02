#include "common.hlsli"

struct constant_data
{
    uint visibility_buffer;
    uint material_tile_buffer;
    uint tile_count_x;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

[numthreads(8, 8, 1)]
void cs_main()
{

}