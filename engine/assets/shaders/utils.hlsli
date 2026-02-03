#ifndef UTILS_HLSLI
#define UTILS_HLSLI

// 2D Polyhedral Bounds of a Clipped, Perspective-Projected 3D Sphere. Michael Mara, Morgan McGuire. 2013
bool project_shpere_perspective(float4 sphere_vs, float p00, float p11, float near, out float4 aabb)
{
    if (sphere_vs.z < sphere_vs.w + near)
    {
        return false;
    }

    float3 cr = sphere_vs.xyz * sphere_vs.w;
    float czr2 = sphere_vs.z * sphere_vs.z - sphere_vs.w * sphere_vs.w;

    float vx = sqrt(sphere_vs.x * sphere_vs.x + czr2);
    float min_x = (vx * sphere_vs.x - cr.z) / (vx * sphere_vs.z + cr.x);
    float max_x = (vx * sphere_vs.x + cr.z) / (vx * sphere_vs.z - cr.x);

    float vy = sqrt(sphere_vs.y * sphere_vs.y + czr2);
    float min_y = (vy * sphere_vs.y - cr.z) / (vy * sphere_vs.z + cr.y);
    float max_y = (vy * sphere_vs.y + cr.z) / (vy * sphere_vs.z - cr.y);

    aabb = float4(min_x * p00, min_y * p11, max_x * p00, max_y * p11);

    return true;
}

bool project_shpere_orthographic(float4 sphere_vs, float p00, float p11, out float4 aabb)
{
    float left = sphere_vs.x - sphere_vs.w;
    float right = sphere_vs.x + sphere_vs.w;
    float bottom = sphere_vs.y - sphere_vs.w;
    float top = sphere_vs.y + sphere_vs.w;

    aabb =  float4(left * p00, bottom * p11, right * p00, top * p11);

    return true;
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