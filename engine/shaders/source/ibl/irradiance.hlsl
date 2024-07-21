#include "violet_define.hlsli"

struct irradiance_data
{
    uint width;
    uint height;
};
ConstantBuffer<irradiance_data> data : register(b0, space0);
TextureCube env_texture : register(t1, space0);
SamplerState env_sampler : register(s1, space0);

struct vs_out
{
    float4 position : SV_POSITION;
    
    float3 right : NORMAL0;
    float3 left : NORMAL1;
    float3 top : NORMAL2;
    float3 bottom : NORMAL3;
    float3 front : NORMAL4;
    float3 back : NORMAL5;
};

struct fs_out
{
    float4 right : SV_TARGET0;
    float4 left : SV_TARGET1;
    float4 top : SV_TARGET2;
    float4 bottom : SV_TARGET3;
    float4 front : SV_TARGET4;
    float4 back : SV_TARGET5;
};

float4 sample_irradiance(float3 normal)
{
    float3 up = float3(0.0, 1.0, 0.0);
    float3 right = normalize(cross(up, normal));
    up = cross(normal, right);

    float delta_phi = TWO_PI / data.width;
    float delta_theta = HALF_PI / data.height;

    float3 color = float3(0.0, 0.0, 0.0);
    uint sample_count = 0;
    for (float phi = 0.0; phi < TWO_PI; phi += delta_phi) {
        for (float theta = 0.0; theta < HALF_PI; theta += delta_theta) {
            float3 temp = cos(phi) * right + sin(phi) * up;
            float3 sample_normal = cos(theta) * normal + sin(theta) * temp;
            color += env_texture.Sample(env_sampler, sample_normal).rgb * cos(theta) * sin(theta);
            ++sample_count;
        }
    }
    return float4(PI * color / float(sample_count), 1.0);
}

fs_out fs_main(vs_out input)
{
    fs_out output;
    output.right = sample_irradiance(normalize(input.right));
    output.left = sample_irradiance(normalize(input.left));
    output.top = sample_irradiance(normalize(input.top));
    output.bottom = sample_irradiance(normalize(input.bottom));
    output.front = sample_irradiance(normalize(input.front));
    output.back = sample_irradiance(normalize(input.back));

    return output;
}