struct ash_object
{
    float4x4 transform_m;
};

struct ash_camera
{
    float3 position;
    float _padding_0;
    float3 direction;
    float _padding_1;

    float4x4 transform_v;
    float4x4 transform_p;
    float4x4 transform_vp;
};