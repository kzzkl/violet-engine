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

    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;

    color = saturate((color * (a * color + b)) / (color * (c * color + d) + e));

    ldr[dtid.xy] = float4(color, 1.0f);
}