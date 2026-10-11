#pragma once

#include "rhi/rhi.hpp"
#include <string>
#include <vector>

namespace violet
{
struct skinned_component
{
    std::vector<std::string> inputs;
    rhi_shader* shader;
};
} // namespace violet