#include "common.hlsli"

struct constant_data
{
    float3 bounding_box_min;
    uint padding0;
    float3 bounding_box_max;
    uint padding1;

    uint3 brick_count;
    uint padding2;

    float4x4 matrix_m;
};
PushConstant(constant_data, constant);

ConstantBuffer<camera_data> camera : register(b0, space1);

static const float3 BOUNDS_COLOR = float3(0.30, 0.95, 0.45);
static const float3 BRICK_COLOR = float3(0.16, 0.36, 0.50);

struct vs_output
{
    float4 position_cs : SV_POSITION;

    // 1 for a line on a bounding box face, 0 for an interior lattice line.
    float boundary : BOUNDARY;
};

// A lattice line lies on the box boundary when both of its free axis
// coordinates are on the border of the lattice.
float is_boundary(uint2 coord, uint2 count)
{
    bool on_border_a = coord.x == 0 || coord.x == count.x;
    bool on_border_b = coord.y == 0 || coord.y == count.y;

    return on_border_a && on_border_b ? 1.0 : 0.0;
}

vs_output vs_main(uint vertex_id : SV_VertexID)
{
    uint3 brick_count;
#ifdef SDF_BOUNDS_BRICK
    brick_count = max(constant.brick_count, uint3(1, 1, 1));
#else
    brick_count = uint3(1, 1, 1);
#endif

    // The lattice has one family of lines per axis. A line is identified by the
    // coordinates of its two free axes, which index the brick boundaries in
    // [0, brick_count].
    uint3 line_count = uint3(
        (brick_count.y + 1) * (brick_count.z + 1),
        (brick_count.x + 1) * (brick_count.z + 1),
        (brick_count.x + 1) * (brick_count.y + 1));

    uint line_index = vertex_id >> 1;
    uint endpoint = vertex_id & 1;

    uint axis = 0;
    uint index = line_index;
    if (index >= line_count.x)
    {
        index -= line_count.x;
        axis = 1;

        if (index >= line_count.y)
        {
            index -= line_count.y;
            axis = 2;
        }
    }

    uint axis_a = (axis + 1) % 3;
    uint axis_b = (axis + 2) % 3;

    uint count_a = brick_count[axis_a];
    uint count_b = brick_count[axis_b];

    uint2 coord = uint2(index / (count_b + 1), index % (count_b + 1));

    // Position inside the bounding box, in [0, 1]. The line spans the whole box
    // along its own axis and sits on one brick boundary of the free axes.
    float3 uvw = 0.0;
    uvw[axis] = (float)endpoint;
    uvw[axis_a] = (float)coord.x / (float)count_a;
    uvw[axis_b] = (float)coord.y / (float)count_b;

    // The SDF bounds are in local space, matrix_m brings them to world space.
    float3 position_ls = lerp(constant.bounding_box_min, constant.bounding_box_max, uvw);
    float3 position_ws = mul(constant.matrix_m, float4(position_ls, 1.0)).xyz;

    vs_output output;
    output.position_cs = mul(camera.matrix_vp, float4(position_ws, 1.0));
    output.boundary = is_boundary(coord, uint2(count_a, count_b));

    return output;
}

float4 fs_main(vs_output input) : SV_TARGET0
{
    // The box edges stand out against the inner brick lattice.
    return float4(lerp(BRICK_COLOR, BOUNDS_COLOR, input.boundary), 1.0);
}
