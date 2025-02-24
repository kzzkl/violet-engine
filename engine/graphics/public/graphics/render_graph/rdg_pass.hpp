#pragma once

#include "graphics/render_graph/rdg_command.hpp"
#include "graphics/render_graph/rdg_reference.hpp"
#include <functional>

namespace violet
{
enum rdg_pass_type
{
    RDG_PASS_RASTER,
    RDG_PASS_COMPUTE,
    RDG_PASS_TRANSFER,
};

class rdg_pass : public rdg_node
{
public:
    rdg_pass(rdg_allocator* allocator) noexcept;

    rdg_texture_ref add_texture(
        rdg_texture* texture,
        rhi_pipeline_stage_flags stages,
        rhi_access_flags access,
        rhi_texture_layout layout,
        std::uint32_t level = 0,
        std::uint32_t level_count = 0,
        std::uint32_t layer = 0,
        std::uint32_t layer_count = 0);
    rdg_texture_srv add_texture_srv(
        rdg_texture* texture,
        rhi_pipeline_stage_flags stages,
        rhi_texture_dimension dimension = RHI_TEXTURE_DIMENSION_2D,
        std::uint32_t level = 0,
        std::uint32_t level_count = 0,
        std::uint32_t layer = 0,
        std::uint32_t layer_count = 0);
    rdg_texture_uav add_texture_uav(
        rdg_texture* texture,
        rhi_pipeline_stage_flags stages,
        rhi_texture_dimension dimension = RHI_TEXTURE_DIMENSION_2D,
        std::uint32_t level = 0,
        std::uint32_t level_count = 0,
        std::uint32_t layer = 0,
        std::uint32_t layer_count = 0);

    rdg_buffer_ref add_buffer(
        rdg_buffer* buffer,
        rhi_pipeline_stage_flags stages,
        rhi_access_flags access,
        std::uint64_t offset = 0,
        std::uint64_t size = 0);
    rdg_buffer_srv add_buffer_srv(
        rdg_buffer* buffer,
        rhi_pipeline_stage_flags stages,
        std::uint64_t offset = 0,
        std::uint64_t size = 0,
        rhi_format texel_format = RHI_FORMAT_UNDEFINED);
    rdg_buffer_uav add_buffer_uav(
        rdg_buffer* buffer,
        rhi_pipeline_stage_flags stages,
        std::uint64_t offset = 0,
        std::uint64_t size = 0,
        rhi_format texel_format = RHI_FORMAT_UNDEFINED);

    // Only for render pass.
    rdg_texture_rtv add_render_target(
        rdg_texture* texture,
        rhi_attachment_load_op load_op = RHI_ATTACHMENT_LOAD_OP_LOAD,
        rhi_attachment_store_op store_op = RHI_ATTACHMENT_STORE_OP_STORE,
        std::uint32_t level = 0,
        std::uint32_t layer = 0,
        rhi_clear_value clear_value = {});
    rdg_texture_dsv set_depth_stencil(
        rdg_texture* texture,
        rhi_attachment_load_op load_op = RHI_ATTACHMENT_LOAD_OP_LOAD,
        rhi_attachment_store_op store_op = RHI_ATTACHMENT_STORE_OP_STORE,
        std::uint32_t level = 0,
        std::uint32_t layer = 0,
        rhi_clear_value clear_value = {});

    rhi_parameter* add_parameter(const rhi_parameter_desc& desc);

    template <typename Functor>
    void each_reference(Functor&& functor)
    {
        for (auto& reference : m_references)
        {
            functor(reference);
        }
    }

    void execute(rdg_command& command);

    void set_pass_type(rdg_pass_type type) noexcept
    {
        m_type = type;
    }

    rdg_pass_type get_pass_type() const noexcept
    {
        return m_type;
    }

    void set_batch_index(std::size_t index) noexcept
    {
        m_batch_index = index;
    }

    std::size_t get_batch_index() const noexcept
    {
        return m_batch_index;
    }

    void reset() noexcept override
    {
        m_references.clear();
        m_batch_index = 0;
        rdg_node::reset();
    }

private:
    virtual void on_execute(rdg_command& command) {}

    rdg_pass_type m_type;

    std::vector<rdg_reference*> m_references;

    std::size_t m_batch_index;

    rdg_allocator* m_allocator;
};

template <typename T>
class rdg_lambda_pass : public rdg_pass
{
public:
    using data_type = T;

    rdg_lambda_pass(rdg_allocator* allocator) noexcept
        : rdg_pass(allocator)
    {
    }

    template <typename Functor>
    void setup(Functor&& functor) noexcept
    {
        functor(m_data, *this);
    }

    template <typename Functor>
    void set_execute(Functor&& functor) noexcept
    {
        m_execute = std::forward<Functor>(functor);
    }

private:
    void on_execute(rdg_command& command) override
    {
        m_execute(m_data, command);
    }

    data_type m_data;

    std::function<void(const data_type&, rdg_command&)> m_execute;
};
} // namespace violet