#pragma once

#include "graphics/render_graph/compute_pipeline.hpp"
#include <cassert>
#include <memory>
#include <vector>

namespace violet
{
class compute_pass : public render_node
{
public:
    compute_pass();
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