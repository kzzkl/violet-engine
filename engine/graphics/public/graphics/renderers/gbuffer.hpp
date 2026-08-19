#pragma once

namespace violet
{
enum gbuffer
{
    GBUFFER_ALBEDO,   // R8G8B8A8_UNORM
    GBUFFER_MATERIAL, // R8G8B8_UNORM, x: Roughness, y: Metallic
    GBUFFER_NORMAL,   // R32_UINT, 24 bit: Octahedron Normal, 8 bit: Shading Model
    GBUFFER_EMISSIVE, // R8G8B8A8_UNORM
};
}