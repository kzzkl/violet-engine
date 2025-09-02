#pragma once

#include "graphics/shading_model.hpp"

namespace violet
{
class pbr_shading_model : public shading_model<pbr_shading_model>
{
public:
    struct constant_data
    {
        std::uint32_t brdf_lut;
    };

    pbr_shading_model();

    const rdg_compute_pipeline& get_pipeline();
    constant_data get_constant() const;

private:
    rdg_compute_pipeline m_pipeline_with_ao{};
    rdg_compute_pipeline m_pipeline_without_ao{};
};
} // namespace violet