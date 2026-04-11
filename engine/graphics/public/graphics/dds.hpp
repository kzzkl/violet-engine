#pragma once

#include "graphics/resources/texture.hpp"

namespace violet
{
class dds
{
public:
    static bool save(std::string_view path, const texture_data& data);
    static bool load(std::string_view path, texture_data& data);
};
} // namespace violet