#pragma once

#include "mesh_loader_impl.hpp"

namespace violet
{
class gltf_loader : public mesh_loader_impl
{
public:
    bool load(std::string_view path, mesh_loader::scene_data& scene_data) override;
};
} // namespace violet