#ifndef COLOR_HLSLI
#define COLOR_HLSLI

#include "common.hlsli"

// https://www.shadertoy.com/view/4dKcWK

float3 hue_to_rgb(float hue)
{
    // Hue [0..1] to RGB [0..1]
    // See http://www.chilliant.com/rgb2hsv.html
    float3 rgb = abs(hue * 6. - float3(3, 2, 4)) * float3(1, -1, -1) + float3(-1, 2, 2);
    return clamp(rgb, 0., 1.);
}

float3 rgb_to_hcv(float3 rgb)
{
    // RGB [0..1] to Hue-Chroma-Value [0..1]
    // Based on work by Sam Hocevar and Emil Persson
    float4 p = (rgb.g < rgb.b) ? float4(rgb.bg, -1., 2. / 3.) : float4(rgb.gb, 0., -1. / 3.);
    float4 q = (rgb.r < p.x) ? float4(p.xyw, rgb.r) : float4(rgb.r, p.yzx);
    float c = q.x - min(q.w, q.y);
    float h = abs((q.w - q.y) / (6. * c + EPSILON) + q.z);
    return float3(h, c, q.x);
}

float3 hsv_to_rgb(float3 hsv)
{
    // Hue-Saturation-Value [0..1] to RGB [0..1]
    float3 rgb = hue_to_rgb(hsv.x);
    return ((rgb - 1.) * hsv.y + 1.) * hsv.z;
}

float3 hsl_to_rgb(float3 hsl)
{
    // Hue-Saturation-Lightness [0..1] to RGB [0..1]
    float3 rgb = hue_to_rgb(hsl.x);
    float c = (1. - abs(2. * hsl.z - 1.)) * hsl.y;
    return (rgb - 0.5) * c + hsl.z;
}

float3 rgb_to_hsv(float3 rgb)
{
    // RGB [0..1] to Hue-Saturation-Value [0..1]
    float3 hcv = rgb_to_hcv(rgb);
    float s = hcv.y / (hcv.z + EPSILON);
    return float3(hcv.x, s, hcv.z);
}

float3 rgb_to_hsl(float3 rgb)
{
    // RGB [0..1] to Hue-Saturation-Lightness [0..1]
    float3 hcv = rgb_to_hcv(rgb);
    float z = hcv.z - hcv.y * 0.5;
    float s = hcv.y / (1. - abs(z * 2. - 1.) + EPSILON);
    return float3(hcv.x, s, z);
}

float3 rgb_to_ycocgr(float3 color_rgb)
{
    float3 color_YCoCgR;

    color_YCoCgR.y = color_rgb.r - color_rgb.b;
    float temp = color_rgb.b + color_YCoCgR.y / 2;
    color_YCoCgR.z = color_rgb.g - temp;
    color_YCoCgR.x = temp + color_YCoCgR.z / 2;

    return color_YCoCgR;
}

float3 ycocgr_to_rgb(float3 color_YCoCgR)
{
    float3 color_rgb;

    float temp = color_YCoCgR.x - color_YCoCgR.z / 2;
    color_rgb.g = color_YCoCgR.z + temp;
    color_rgb.b = temp - color_YCoCgR.y / 2;
    color_rgb.r = color_rgb.b + color_YCoCgR.y;

    return color_rgb;
}

#endif