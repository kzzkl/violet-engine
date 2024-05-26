#pragma once

#include "graphics/material.hpp"
#include "graphics/render_graph/render_graph.hpp"
#include "math/math.hpp"
#include <array>

namespace violet::sample
{
class preprocess_graph : public render_graph
{
public:
    preprocess_graph();

private:
    std::array<rhi_ptr<rhi_texture>, 6> m_irradiance_targets;
};

class pbr_material : public material
{
public:
    // pbr_material(renderer* renderer, material_layout* layout);

    void set_albedo(const float3& albedo);
    void set_metalness(float metalness);
    void set_roughness(float roughness);
};

class pbr_pass : public rdg_render_pass
{
public:
    constexpr static rhi_parameter_desc get_material_layout()
    {
        rhi_parameter_desc desc = {};
        desc.bindings[0] = {
            .type = RHI_PARAMETER_TYPE_TEXTURE,
            .stage = RHI_PARAMETER_STAGE_FLAG_FRAGMENT,
            .size = 1}; // albedo
        desc.bindings[1] = {
            .type = RHI_PARAMETER_TYPE_TEXTURE,
            .stage = RHI_PARAMETER_STAGE_FLAG_FRAGMENT,
            .size = 1}; // emissive
        desc.bindings[2] = {
            .type = RHI_PARAMETER_TYPE_TEXTURE,
            .stage = RHI_PARAMETER_STAGE_FLAG_FRAGMENT,
            .size = 1}; // metal roughness
        desc.bindings[3] = {
            .type = RHI_PARAMETER_TYPE_TEXTURE,
            .stage = RHI_PARAMETER_STAGE_FLAG_FRAGMENT,
            .size = 1}; // normal
        desc.binding_count = 4;
        return desc;
    };

public:
    pbr_pass();

    virtual void execute(rhi_command* command, rdg_context* context) override;

private:
    rdg_pass_reference* m_render_target;
};

class pbr_render_graph : public render_graph
{
public:
    pbr_render_graph(rhi_format render_target_format);
};
} // namespace violet::sample