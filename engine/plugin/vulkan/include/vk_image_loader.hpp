#pragma once

#include "vk_common.hpp"
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace violet::graphics::vk
{
struct vk_image_data
{
    std::uint32_t width;
    std::uint32_t height;

    std::uint32_t channels;
    VkFormat format;

    std::vector<char> pixels;
};

class vk_image_loader
{
public:
    bool load(std::string_view file);

    const vk_image_data& mipmap(std::size_t level) const { return m_mipmap[level]; }

private:
    bool load_dds(std::string_view file);
    bool load_other(std::string_view file);

    std::vector<vk_image_data> m_mipmap;
};
} // namespace violet::graphics::vk