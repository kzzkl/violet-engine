cbuffer ash_object : register(b0, space0)
{
    float4x4 transform_m;
};

cbuffer ash_shadow : register(b0, space1)
{
    float4x4 light_vp;
};

struct vs_in
{
    float3 position : POSITION;
};

struct vs_out
{
    float4 position : SV_POSITION;
};

vs_out vs_main(vs_in vin)
{
    vs_out result;
    result.position = mul(mul(float4(vin.position, 1.0f), transform_m), light_vp);
    return result;
}

float4 ps_main(vs_out pin) : SV_TARGET
{
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}