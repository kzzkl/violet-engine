#pragma once

#include "mesh_loader.hpp"

namespace violet::sample
{
class gltf_loader : public mesh_loader
{
public:
    gltf_loader();
    virtual ~gltf_loader();

    std::optional<scene_data> load(std::string_view path) override;
};
} // namespace violet::sample