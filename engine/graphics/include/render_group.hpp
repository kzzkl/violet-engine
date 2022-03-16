#pragma once

#include "graphics_interface.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include <vector>

namespace ash::graphics
{
class render_group
{
public:
    render_group(pipeline_parameter_layout* layout, pipeline* pipeline);

    void add(mesh* mesh) { m_meshs.push_back(mesh); }
    void clear() { m_meshs.clear(); }

    pipeline* get_pipeline() const noexcept { return m_pipeline.get(); }
    pipeline_parameter_layout* get_layout() const noexcept { return m_layout.get(); }

    auto begin() noexcept { return m_meshs.begin(); }
    auto end() noexcept { return m_meshs.end(); }

private:
    std::unique_ptr<pipeline_parameter_layout> m_layout;
    std::unique_ptr<pipeline> m_pipeline;

    std::vector<mesh*> m_meshs;
};
} // namespace ash::graphics