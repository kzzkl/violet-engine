#ifndef UTILS_HLSLI
#define UTILS_HLSLI

// 2D Polyhedral Bounds of a Clipped, Perspective-Projected 3D Sphere. Michael Mara, Morgan McGuire. 2013
float4 project_shpere_vs(float4 sphere, float p00, float p11)
{
    float3 cr = sphere.xyz * sphere.w;
    float czr2 = sphere.z * sphere.z - sphere.w * sphere.w;

    float vx = sqrt(sphere.x * sphere.x + czr2);
    float min_x = (vx * sphere.x - cr.z) / (vx * sphere.z + cr.x);
    float max_x = (vx * sphere.x + cr.z) / (vx * sphere.z - cr.x);

    float vy = sqrt(sphere.y * sphere.y + czr2);
    float min_y = (vy * sphere.y - cr.z) / (vy * sphere.z + cr.y);
    float max_y = (vy * sphere.y + cr.z) / (vy * sphere.z - cr.y);

    float4 aabb = float4(min_x * p00, min_y * p11, max_x * p00, max_y * p11);

    return aabb;
}

float3 to_color(uint index)
{
    uint hash = index + 1;
    hash ^= hash >> 16;
    hash *= 0x85ebca6b;
    hash ^= hash >> 13;
    hash *= 0xc2b2ae35;
    hash ^= hash >> 16;
    return float3((hash >> 16) & 0xFF, (hash >> 8) & 0xFF, hash & 0xFF) / 255.0;
}

#endif