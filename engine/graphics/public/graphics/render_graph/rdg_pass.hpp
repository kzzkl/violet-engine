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
    RDG_PASS_TYPE_RENDER,
    RDG_PASS_TYPE_COMPUTE,
    RDG_PASS_TYPE_OTHER
};

enum rdg_pass_parameter_flag
{
    RDG_PASS_PARAMETER_FLAG_NONE,
    RDG_PASS_PARAMETER_FLAG_MATERIAL,
    RDG_PASS_PARAMETER_FLAG_PASS
};
using rdg_pass_parameter_flags = std::uint32_t;

class rdg_pass : public rdg_node
{
public:
    rdg_pass();
    virtual ~rdg_pass();

    rdg_reference* add_texture(
        rdg_texture* texture,
        rhi_access_flags access,
        rhi_texture_layout layout);
    rdg_reference* add_buffer(rdg_buffer* buffer, rhi_access_flags access);

    std::vector<rdg_reference*> get_references(rhi_access_flags access) const;
    std::vector<rdg_reference*> get_references(rdg_reference_type type) const;
    std::vector<rdg_reference*> get_references() const;

    template <typename Functor>
    void set_execute(Functor functor)
    {
        m_executor = functor;
    }

    void execute(rdg_command* command) { m_executor(command); }

    virtual rdg_pass_type get_type() const noexcept { return RDG_PASS_TYPE_OTHER; }

protected:
    rdg_reference* add_reference(rdg_resource* resource);

private:
    std::vector<std::unique_ptr<rdg_reference>> m_references;
    std::function<void(rdg_command*)> m_executor;
};

class rdg_render_pass : public rdg_pass
{
public:
    rdg_render_pass();

    void add_render_target(
        rdg_texture* render_target,
        rhi_attachment_load_op load_op = RHI_ATTACHMENT_LOAD_OP_LOAD,
        rhi_attachment_store_op store_op = RHI_ATTACHMENT_STORE_OP_STORE,
        const rhi_attachment_blend& blend = {});
    void set_depth_stencil(
        rdg_texture* depth_stencil,
        rhi_attachment_load_op load_op = RHI_ATTACHMENT_LOAD_OP_LOAD,
        rhi_attachment_store_op store_op = RHI_ATTACHMENT_STORE_OP_STORE);

    virtual rdg_pass_type get_type() const noexcept final { return RDG_PASS_TYPE_RENDER; }

    const std::vector<rdg_reference*>& get_attachments() const noexcept { return m_attachments; }

private:
    std::vector<rdg_reference*> m_attachments;
};

class rdg_compute_pass : public rdg_pass
{
public:
    rdg_compute_pass();

    virtual rdg_pass_type get_type() const noexcept final { return RDG_PASS_TYPE_COMPUTE; }
};
} // namespace violet