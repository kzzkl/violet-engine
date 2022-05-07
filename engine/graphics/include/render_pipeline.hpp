#pragma once

#include "camera.hpp"
#include "graphics_interface.hpp"
#include "render_parameter.hpp"
#include <vector>

namespace ash::graphics
{
class render_pass;
struct render_unit
{
    resource_interface* vertex_buffer{nullptr};
    resource_interface* index_buffer{nullptr};

    std::size_t index_start{0};
    std::size_t index_end{0};
    std::size_t vertex_base{0};

    render_pass* render_pass{nullptr};
    std::vector<pipeline_parameter*> parameters;

    void* external{nullptr};
};

class render_pass
{
public:
    render_pass();
    virtual ~render_pass() = default;

    void add(const render_unit* unit) { m_units.push_back(unit); }
    void clear() { m_units.clear(); }

    virtual void render(const camera& camera, render_command_interface* command) = 0;

protected:
    const std::vector<const render_unit*>& units() const { return m_units; }

private:
    std::vector<const render_unit*> m_units;
};
} // namespace ash::graphics