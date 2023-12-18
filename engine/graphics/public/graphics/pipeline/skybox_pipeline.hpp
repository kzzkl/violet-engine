#pragma once

#include "graphics/render_graph/render_pipeline.hpp"

namespace violet
{
class skybox_pipeline : public render_pipeline
{
public:
    skybox_pipeline();

    virtual bool compile(compile_context& context) override;

private:
    virtual void render(std::vector<render_mesh>& meshes, const execute_context& context) override;
};
} // namespace violet