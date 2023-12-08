#pragma once

#include "graphics/render_graph/compute_pipeline.hpp"
#include <memory>
#include <vector>

namespace violet
{
class compute_pass : public render_node
{
public:
    compute_pass(std::string_view name, renderer* renderer);
    virtual ~compute_pass();

    template <typename T, typename... Args>
    T* add_pipeline(std::string_view name, Args&&... args)
    {
        auto pipeline = std::make_unique<T>(name, get_renderer(), std::forward<Args>(args)...);
        T* result = pipeline.get();

        m_pipelines.push_back(std::move(pipeline));
        return result;
    }
    compute_pipeline* get_pipeline(std::string_view name) const;

    bool compile();
    void execute(rhi_render_command* command);

private:
    std::vector<std::unique_ptr<compute_pipeline>> m_pipelines;
};
} // namespace violet