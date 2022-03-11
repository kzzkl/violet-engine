#pragma once

#include <cstdint>

#define PLUGIN_API __declspec(dllexport)

namespace ash::core
{
struct plugin_version
{
    uint32_t major;
    uint32_t minor;
};

struct plugin_info
{
    char name[64];
    plugin_version version;
};

using get_plugin_info = plugin_info (*)();
} // namespace ash::core