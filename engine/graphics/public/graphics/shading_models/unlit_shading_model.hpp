#pragma once

#include "graphics/shading_model.hpp"

namespace violet
{
class unlit_shading_model : public shading_model<unlit_shading_model>
{
public:
    unlit_shading_model();

    const rdg_compute_pipeline& get_pipeline()
    {
        return m_pipeline;
    }

private:
    rdg_compute_pipeline m_pipeline;
};
} // namespace violet