#pragma once

#include <vector>

namespace violet
{
class mesh;
class render_pipeline
{
public:
    virtual void render(const std::vector<mesh*>& meshes) = 0;
};
} // namespace violet