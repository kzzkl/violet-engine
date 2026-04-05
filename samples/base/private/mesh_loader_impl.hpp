#pragma once

#include "sample/mesh_loader.hpp"

namespace violet
{
class mesh_loader_impl
{
public:
    virtual ~mesh_loader_impl() = default;

    virtual bool load(std::string_view path, mesh_loader::scene_data& scene_data) = 0;
};
} // namespace violet