#pragma once

#include "mesh_loader.hpp"

namespace violet::sample
{
class gltf_loader : public mesh_loader
{
public:
    gltf_loader(std::string_view path);
    virtual ~gltf_loader();

    std::optional<scene_data> load() override;

private:
    std::string m_path;
};
} // namespace violet::sample