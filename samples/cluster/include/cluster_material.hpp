#pragma once

#include "graphics/material.hpp"

namespace violet
{
struct cluster_material_constant
{
};

class cluster_material : public mesh_material<cluster_material_constant>
{
public:
    cluster_material();
};
} // namespace violet