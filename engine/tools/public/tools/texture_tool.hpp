#pragma once

#include "graphics/resources/texture.hpp"

namespace violet
{
class texture_tool
{
public:
    static bool load(std::string_view path, texture_data& data);
    static bool compress(const texture_data& src, texture_data& dst);
    static bool generate_mipmaps(const texture_data& src, texture_data& dst);
};
} // namespace violet