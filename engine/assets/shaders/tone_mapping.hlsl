struct tone_mapping_data
{
    uint hdr;
    uint ldr;
};
ConstantBuffer<tone_mapping_data> tone_mapping : register(b0, space1);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    Texture2D<float4> hdr = ResourceDescriptorHeap[tone_mapping.hdr];
    RWTexture2D<float4> ldr = ResourceDescriptorHeap[tone_mapping.ldr];

    float3 color = hdr[dtid.xy].rgb;

    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;

    color = saturate((color * (a * color + b)) / (color * (c * color + d) + e));

    ldr[dtid.xy] = float4(color, 1.0);
}