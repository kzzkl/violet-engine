#pragma once

#include "graphics/render_pipeline.hpp"
#include "ui/control_mesh.hpp"

namespace violet::ui
{
class mvp_pipeline_parameter : public graphics::pipeline_parameter
{
public:
    static constexpr graphics::pipeline_parameter_desc layout = {
        .parameters = {{graphics::PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER, sizeof(math::float4x4)}},
        .parameter_count = 1};

public:
    mvp_pipeline_parameter();

    void mvp_matrix(const math::float4x4& mvp);
};

class offset_pipeline_parameter : public graphics::pipeline_parameter
{
public:
    static constexpr graphics::pipeline_parameter_desc layout = {
        .parameters =
            {{graphics::PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER, 1024 * sizeof(math::float4)}},
        .parameter_count = 1};

public:
    offset_pipeline_parameter();

    void offset(const std::vector<math::float4>& offset);
};

class material_pipeline_parameter : public graphics::pipeline_parameter
{
public:
    static constexpr graphics::pipeline_parameter_desc layout = {
        .parameters = {
            {graphics::PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER, sizeof(std::uint32_t)},
            {graphics::PIPELINE_PARAMETER_TYPE_SHADER_RESOURCE, 1}},
        .parameter_count = 2
    };

public:
    material_pipeline_parameter();

    void mesh_type(control_mesh_type type);
    void texture(graphics::rhi_resource* texture);
};

class ui_pipeline : public graphics::render_pipeline
{
public:
    ui_pipeline();

    void set_mvp_matrix(const math::float4x4& mvp);
    void set_offset(const std::vector<math::float4>& offset);

    virtual void render(
        const graphics::render_context& context,
        graphics::rhi_render_command* command) override;

private:
    std::unique_ptr<graphics::rhi_render_pipeline> m_interface;

    std::unique_ptr<mvp_pipeline_parameter> m_mvp_parameter;
    std::unique_ptr<offset_pipeline_parameter> m_offset_parameter;
};
} // namespace violet::ui