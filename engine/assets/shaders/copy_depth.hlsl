struct copy_data
{
    uint src;
    uint dst;
};
ConstantBuffer<copy_data> copy : register(b0, space1);

[numthreads(8, 8, 1)]
void cs_main(uint3 id : SV_DispatchThreadID)
{
    Texture2D<float> src = ResourceDescriptorHeap[copy.src];
    RWTexture2D<float> dst = ResourceDescriptorHeap[copy.dst];

    dst[id.xy] = src[id.xy];
}