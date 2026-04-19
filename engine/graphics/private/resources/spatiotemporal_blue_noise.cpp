#include "graphics/resources/spatiotemporal_blue_noise.hpp"
#include "graphics/texture_loader.hpp"

namespace violet
{
spatiotemporal_blue_noise::spatiotemporal_blue_noise()
{
    auto texture = texture_loader::load("assets/textures/stbn_vec3_2Dx1D_128x128x64.dds");
    if (texture == nullptr)
    {
        throw std::runtime_error("Failed to load assets/textures/stbn_vec3_2Dx1D_128x128x64.dds.");
    }

    set_texture(std::move(texture));
}

spatiotemporal_blue_noise_cosine::spatiotemporal_blue_noise_cosine()
{
    auto texture =
        texture_loader::load("assets/textures/stbn_unitvec3_cosine_2Dx1D_128x128x64.dds");
    if (texture == nullptr)
    {
        throw std::runtime_error(
            "Failed to load assets/textures/stbn_unitvec3_cosine_2Dx1D_128x128x64.dds.");
    }

    set_texture(std::move(texture));
}
} // namespace violet