#pragma once

#include "graphics/render_graph/render_pipeline.hpp"

namespace violet
{
class basic_pipeline : public render_pipeline
{
public:
    basic_pipeline();

private:
    virtual void render(std::vector<render_mesh>& meshes, const execute_context& context) override;
};
} // namespace violet