#pragma once

#include <cstdint>

#define PLUGIN_API __declspec(dllexport)

namespace violet
{
struct plugin_version
{
    std::uint32_t major;
    std::uint32_t minor;
};

struct plugin_info
{
    char name[64];
    plugin_version version;
};

using get_plugin_info = plugin_info (*)();
} // namespace violet