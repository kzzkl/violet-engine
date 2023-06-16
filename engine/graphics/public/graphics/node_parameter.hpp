#pragma once

#include "graphics/pipeline_parameter.hpp"

namespace violet
{
class node_parameter : public pipeline_parameter
{
public:
    static constexpr rhi_pipeline_parameter_desc layout = {
        .parameters = {{RHI_PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER, sizeof(float4x4)}},
        .parameter_count = 1};

public:
    node_parameter();

    void set_world_matrix(const float4x4& matrix);
};
} // namespace violet