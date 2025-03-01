#include "graphics/resources/hilbert_lut.hpp"
#include "tools/texture_loader.hpp"

namespace violet
{
hilbert_lut::hilbert_lut(std::uint32_t level)
{
    std::size_t width = 1 << level;

    std::vector<std::uint32_t> hilbert_indexes(width * width);
    for (std::uint32_t y = 0; y < width; ++y)
    {
        for (std::uint32_t x = 0; x < width; ++x)
        {
            hilbert_indexes[y * width + x] = hilbert_index(x, y, level);
        }
    }

    texture_data data = {
        .format = RHI_FORMAT_R32_UINT,
    };
    data.mipmaps.resize(1);
    data.mipmaps[0].extent.height = width;
    data.mipmaps[0].extent.width = width;
    data.mipmaps[0].pixels.resize(4ull * width * width);

    std::memcpy(
        data.mipmaps[0].pixels.data(),
        hilbert_indexes.data(),
        hilbert_indexes.size() * sizeof(std::uint32_t));

    set_texture(texture_loader::load(data));
}

std::uint32_t hilbert_lut::hilbert_index(std::uint32_t x, std::uint32_t y, std::uint32_t level)
{
    std::uint32_t width = 1 << level;

    std::uint32_t index = 0;
    for (std::uint32_t curr_level = width / 2; curr_level > 0; curr_level /= 2)
    {
        std::uint32_t region_x = (x & curr_level) > 0;
        std::uint32_t region_y = (y & curr_level) > 0;
        index += curr_level * curr_level * ((3 * region_x) ^ region_y);
        if (region_y == 0)
        {
            if (region_x == 1)
            {
                x = width - 1 - x;
                y = width - 1 - y;
            }

            std::uint32_t temp = x;
            x = y;
            y = temp;
        }
    }
    return index;
}
} // namespace violet