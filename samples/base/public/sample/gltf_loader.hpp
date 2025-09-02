#pragma once

#include "mesh_loader.hpp"

namespace violet
{
class gltf_loader : public mesh_loader
{
public:
    std::optional<scene_data> load(std::string_view path) override;
};
} // namespace violet