#include "violet_common.hlsli"

ConstantBuffer<violet_object> object : register(b0, space0);

struct violet_shadow
{
    float4x4 light_vp;
};
ConstantBuffer<violet_shadow> shadow : register(b0, space1);

struct vs_input
{
    float3 position : POSITION;
};

struct vs_output
{
    float4 position : SV_POSITION;
};

vs_output fs_main(vs_input input)
{
    vs_output result;
    result.position = mul(mul(float4(input.position, 1.0f), object.transform_m), shadow.light_vp);
    // result.position = mul(float4(vin.position, 1.0f), shadow.light_vp);
    return result;
}