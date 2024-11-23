#ifndef GLOBAL_HLSLI
#define GLOBAL_HLSLI

ByteAddressBuffer constant : register(t0, space0);

template <typename T>
T load_constant(uint address)
{
    return constant.Load<T>(address);
}

#endif