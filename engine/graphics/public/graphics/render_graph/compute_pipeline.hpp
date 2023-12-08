#pragma once

#include "graphics/render_graph/render_node.hpp"

namespace violet
{
struct dispatch_data
{
    std::size_t x;
    std::size_t y;
    std::size_t z;
    std::vector<rhi_parameter*> parameters;
};

using compute_data = std::vector<dispatch_data>;

class compute_pipeline : public render_node
{
public:
    compute_pipeline(std::string_view name, renderer* renderer);
    virtual ~compute_pipeline();

    void set_shader(std::string_view compute);

    void set_parameter_layouts(const std::vector<rhi_parameter_layout*>& parameter_layouts);
    rhi_parameter_layout* get_parameter_layout(std::size_t index) const noexcept;

    bool compile();
    void execute(rhi_render_command* command);

    void add_dispatch(
        std::size_t x,
        std::size_t y,
        std::size_t z,
        const std::vector<rhi_parameter*>& parameters);

private:
    virtual void compute(rhi_render_command* command, const compute_data& data) = 0;

    std::string m_compute_shader;
    std::vector<rhi_parameter_layout*> m_parameter_layouts;

    compute_data m_compute_data;

    rhi_ptr<rhi_compute_pipeline> m_interface;
};
} // namespace violet