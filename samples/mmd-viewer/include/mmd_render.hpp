#pragma once

#include "graphics/material.hpp"
#include "math/math.hpp"

namespace violet::sample
{
struct mmd_skinning_bone
{
    float4x3 offset;
    float4 quaternion;
};

class mmd_material : public material
{
public:
    mmd_material(render_device* device, material_layout* layout);

    void set_diffuse(const float4& diffuse);
    void set_specular(float3 specular, float specular_strength);
    void set_ambient(const float3& ambient);
    void set_toon_mode(std::uint32_t toon_mode);
    void set_spa_mode(std::uint32_t spa_mode);

    void set_tex(rhi_texture* texture, rhi_sampler* sampler);
    void set_toon(rhi_texture* texture, rhi_sampler* sampler);
    void set_spa(rhi_texture* texture, rhi_sampler* sampler);

    void set_edge(const float4& edge_color, float edge_size);
};

class mmd_edge_material : public material
{
public:
    mmd_edge_material(render_device* device, material_layout* layout);

    void set_edge(const float4& edge_color, float edge_size);
};

class mmd_render_graph : public render_graph
{
public:
    mmd_render_graph(rhi_format render_target_format);

    material_layout* get_material_layout() const noexcept { return m_material_layout.get(); }

private:
    std::unique_ptr<material_layout> m_material_layout;
};

namespace detail
{
constexpr rhi_parameter_desc get_material_parameter_layout()
{
    rhi_parameter_desc desc = {};
    desc.bindings[0] = {
        .type = RHI_PARAMETER_TYPE_UNIFORM_BUFFER,
        .stage = RHI_PARAMETER_STAGE_FLAG_FRAGMENT,
        .size = 72}; // data
    desc.bindings[1] = {
        .type = RHI_PARAMETER_TYPE_TEXTURE,
        .stage = RHI_PARAMETER_STAGE_FLAG_FRAGMENT,
        .size = 1}; // tex
    desc.bindings[2] = {
        .type = RHI_PARAMETER_TYPE_TEXTURE,
        .stage = RHI_PARAMETER_STAGE_FLAG_FRAGMENT,
        .size = 1}; // toon
    desc.bindings[3] = {
        .type = RHI_PARAMETER_TYPE_TEXTURE,
        .stage = RHI_PARAMETER_STAGE_FLAG_FRAGMENT,
        .size = 1}; // spa
    desc.binding_count = 4;

    return desc;
}

constexpr rhi_parameter_desc get_edge_material_parameter_layout()
{
    rhi_parameter_desc desc = {};
    desc.bindings[0] = {
        .type = RHI_PARAMETER_TYPE_UNIFORM_BUFFER,
        .stage = RHI_PARAMETER_STAGE_FLAG_VERTEX | RHI_PARAMETER_STAGE_FLAG_FRAGMENT,
        .size = 20}; // data
    desc.binding_count = 1;

    return desc;
}

constexpr rhi_parameter_desc get_skeleton_parameter_layout()
{
    rhi_parameter_desc desc = {};
    desc.bindings[0] = {
        .type = RHI_PARAMETER_TYPE_STORAGE_BUFFER,
        .stage = RHI_PARAMETER_STAGE_FLAG_COMPUTE,
        .size = 1}; // in position
    desc.bindings[1] = {
        .type = RHI_PARAMETER_TYPE_STORAGE_BUFFER,
        .stage = RHI_PARAMETER_STAGE_FLAG_COMPUTE,
        .size = 1}; // in normal
    desc.bindings[2] = {
        .type = RHI_PARAMETER_TYPE_STORAGE_BUFFER,
        .stage = RHI_PARAMETER_STAGE_FLAG_COMPUTE,
        .size = 1}; // out position
    desc.bindings[3] = {
        .type = RHI_PARAMETER_TYPE_STORAGE_BUFFER,
        .stage = RHI_PARAMETER_STAGE_FLAG_COMPUTE,
        .size = 1}; // out normal
    desc.bindings[4] = {
        .type = RHI_PARAMETER_TYPE_UNIFORM_BUFFER,
        .stage = RHI_PARAMETER_STAGE_FLAG_COMPUTE,
        .size = sizeof(mmd_skinning_bone) * 1024}; // skeleton
    desc.bindings[5] = {
        .type = RHI_PARAMETER_TYPE_STORAGE_BUFFER,
        .stage = RHI_PARAMETER_STAGE_FLAG_COMPUTE,
        .size = 1}; // bedf
    desc.bindings[6] = {
        .type = RHI_PARAMETER_TYPE_STORAGE_BUFFER,
        .stage = RHI_PARAMETER_STAGE_FLAG_COMPUTE,
        .size = 1}; // sedf
    desc.bindings[7] = {
        .type = RHI_PARAMETER_TYPE_STORAGE_BUFFER,
        .stage = RHI_PARAMETER_STAGE_FLAG_COMPUTE,
        .size = 1}; // data
    desc.bindings[8] = {
        .type = RHI_PARAMETER_TYPE_STORAGE_BUFFER,
        .stage = RHI_PARAMETER_STAGE_FLAG_COMPUTE,
        .size = 1}; // morph
    desc.binding_count = 9;

    return desc;
}
} // namespace detail

struct mmd_parameter_layout
{
    static constexpr rhi_parameter_desc material = detail::get_material_parameter_layout();
    static constexpr rhi_parameter_desc edge_material =
        detail::get_edge_material_parameter_layout();
    static constexpr rhi_parameter_desc skeleton = detail::get_skeleton_parameter_layout();
};
} // namespace violet::sample