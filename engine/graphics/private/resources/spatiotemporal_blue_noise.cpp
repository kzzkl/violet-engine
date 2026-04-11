#include "graphics/resources/spatiotemporal_blue_noise.hpp"
#include "graphics/texture_loader.hpp"

namespace violet
{
spatiotemporal_blue_noise::spatiotemporal_blue_noise()
{
    auto texture = texture_loader::load("assets/textures/stbn_scalar_2Dx1Dx1D_128x128x64x1.dds");
    if (texture == nullptr)
    {
        throw std::runtime_error("Failed to load spatiotemporal blue noise texture.");
    }

    set_texture(std::move(texture));
}
} // namespace violet