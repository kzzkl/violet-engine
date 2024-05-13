#pragma once

#include "graphics/render_graph/pass.hpp"
#include <vector>

namespace violet
{
class material_layout
{
public:
    material_layout(const std::vector<mesh_pass*>& passes);

    const std::vector<mesh_pass*>& get_passes() const noexcept { return m_passes; }

private:
    std::vector<mesh_pass*> m_passes;
};

class material
{
public:
    material(renderer* renderer, material_layout* layout);

    const std::vector<mesh_pass*>& get_passes() const noexcept { return m_layout->get_passes(); }
    rhi_parameter* get_parameter(std::size_t pass_index) const
    {
        return m_parameters[pass_index].get();
    }

private:
    std::vector<rhi_ptr<rhi_parameter>> m_parameters;
    material_layout* m_layout;
};
} // namespace violet