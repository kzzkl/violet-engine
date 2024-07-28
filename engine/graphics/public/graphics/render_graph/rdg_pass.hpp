#pragma once

#include "graphics/render_device.hpp"
#include "graphics/render_graph/rdg_command.hpp"
#include "graphics/render_graph/rdg_resource.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace violet
{
enum rdg_pass_type
{
    RDG_PASS_RENDER,
    RDG_PASS_COMPUTE,
    RDG_PASS_OTHER
};

class rdg_pass : public rdg_node
{
public:
    rdg_pass();
    virtual ~rdg_pass();

    rdg_reference* add_texture(
        rdg_texture* texture,
        rhi_texture_layout layout,
        rhi_pipeline_stage_flags stages,
        rhi_access_flags access);
    rdg_reference* add_buffer(
        rdg_buffer* buffer,
        rhi_pipeline_stage_flags stages,
        rhi_access_flags access);

    std::vector<rdg_reference*> get_references(rhi_access_flags access) const;
    std::vector<rdg_reference*> get_references(rdg_reference_type type) const;
    std::vector<rdg_reference*> get_references() const;

    template <typename Functor>
    void set_execute(Functor functor)
    {
        m_executor = functor;
    }

    void execute(rdg_command* command) { m_executor(command); }

    virtual rdg_pass_type get_type() const noexcept { return RDG_PASS_OTHER; }

protected:
    rdg_reference* add_reference(rdg_resource* resource);

private:
    std::vector<std::unique_ptr<rdg_reference>> m_references;
    std::function<void(rdg_command*)> m_executor;
};

class rdg_attachment
{
public:
    rdg_attachment() noexcept {}
    explicit rdg_attachment(
        rdg_texture* texture,
        rhi_attachment_load_op load_op = RHI_ATTACHMENT_LOAD_OP_LOAD,
        rhi_attachment_store_op store_op = RHI_ATTACHMENT_STORE_OP_STORE) noexcept
        : m_texture(texture),
          m_load_op(load_op),
          m_store_op(store_op)
    {
    }

    rdg_attachment& set_texture(rdg_texture* texture) noexcept
    {
        m_texture = texture;
        return *this;
    }

    rdg_texture* get_texture() const noexcept { return m_texture; }

    rdg_attachment& set_load_op(rhi_attachment_load_op load_op) noexcept
    {
        m_load_op = load_op;
        return *this;
    }

    rhi_attachment_load_op get_load_op() const noexcept { return m_load_op; }

    rdg_attachment& set_store_op(rhi_attachment_store_op store_op) noexcept
    {
        m_store_op = store_op;
        return *this;
    }

    rhi_attachment_store_op get_store_op() const noexcept { return m_store_op; }

private:
    rdg_texture* m_texture{nullptr};
    rhi_attachment_load_op m_load_op{RHI_ATTACHMENT_LOAD_OP_LOAD};
    rhi_attachment_store_op m_store_op{RHI_ATTACHMENT_STORE_OP_STORE};
};

class rdg_render_pass : public rdg_pass
{
public:
    rdg_render_pass();

    void add_render_target(const rdg_attachment& render_target);
    void set_depth_stencil(const rdg_attachment& depth_stencil);

    virtual rdg_pass_type get_type() const noexcept final { return RDG_PASS_RENDER; }

    const std::vector<rdg_reference*>& get_attachments() const noexcept { return m_attachments; }

private:
    std::vector<rdg_reference*> m_attachments;
};

class rdg_compute_pass : public rdg_pass
{
public:
    rdg_compute_pass();

    virtual rdg_pass_type get_type() const noexcept final { return RDG_PASS_COMPUTE; }
};
} // namespace violet