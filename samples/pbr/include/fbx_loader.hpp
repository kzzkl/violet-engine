#pragma once

#include "graphics/geometry.hpp"

namespace violet::sample
{
class fbx_loader
{
public:
    static std::unique_ptr<geometry> load(std::string_view path, vec3f* center = nullptr);
};
} // namespace violet::sample