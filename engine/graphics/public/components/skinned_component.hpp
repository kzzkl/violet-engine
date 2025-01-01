#pragma once

#include "graphics/render_interface.hpp"
#include <string>
#include <vector>

namespace violet
{
struct skinned_component
{
    std::vector<std::string> inputs;
    std::vector<std::pair<std::string, rhi_format>> outputs;
    rhi_shader* shader;
};
} // namespace violet