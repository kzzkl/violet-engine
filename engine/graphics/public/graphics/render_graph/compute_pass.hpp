#pragma once

#include "graphics/render_graph/pass.hpp"
#include <cassert>
#include <memory>
#include <vector>

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

class compute_pipeline
{
public:
    compute_pipeline();
    virtual ~compute_pipeline();

    void set_shader(std::string_view compute);

    void set_parameter_layouts(const std::vector<rhi_parameter_layout*>& parameter_layouts);
    rhi_parameter_layout* get_parameter_layout(std::size_t index) const noexcept;

    virtual bool compile(compile_context& context);

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

class compute_pass : public pass
{
public:
    compute_pass(renderer* renderer, setup_context& context);
    virtual ~compute_pass();

    template <typename T, typename... Args>
    T* add_pipeline(std::string_view name, Args&&... args)
    {
        assert(m_pipeline_map.find(name.data()) == m_pipeline_map.end());

        auto pipeline = std::make_unique<T>(std::forward<Args>(args)...);
        T* result = pipeline.get();

        m_pipelines.push_back(std::move(pipeline));
        m_pipeline_map[name.data()] = m_pipelines.back().get();
        return result;
    }
    compute_pipeline* get_pipeline(std::string_view name) const;

    void add_pipeline_barrier();

    virtual bool compile(compile_context& context) override;
    virtual void execute(execute_context& context) override;

private:
    std::map<std::string, compute_pipeline*> m_pipeline_map;
    std::vector<std::unique_ptr<compute_pipeline>> m_pipelines;
};
} // namespace violet