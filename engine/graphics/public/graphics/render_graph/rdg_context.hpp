#pragma once

#include "graphics/render_device.hpp"
#include <vector>

namespace violet
{
struct rdg_mesh
{
    rhi_parameter* material;
    rhi_parameter* transform;

    std::size_t vertex_start;
    std::size_t vertex_count;
    std::size_t index_start;
    std::size_t index_count;

    rhi_buffer** vertex_buffers;
    rhi_buffer* index_buffer;
};

struct rdg_dispatch
{
    rhi_parameter* parameter;

    std::uint32_t x;
    std::uint32_t y;
    std::uint32_t z;
};

class rdg_pass;
class rdg_context
{
public:
    rdg_context(
        std::size_t resource_count,
        const std::vector<rdg_pass*>& passes,
        render_device* device);

    void set_texture(std::size_t index, rhi_texture* texture);
    rhi_texture* get_texture(std::size_t index);
    rhi_texture* get_texture(rdg_pass* pass, std::size_t reference_index);

    void set_buffer(std::size_t index, rhi_buffer* buffer);
    rhi_buffer* get_buffer(std::size_t index);

    void set_camera(rhi_parameter* camera) noexcept { m_camera = camera; }
    rhi_parameter* get_camera() const noexcept { return m_camera; }

    void set_light(rhi_parameter* light) noexcept { m_light = light; }
    rhi_parameter* get_light() const noexcept { return m_light; }

    void get_parameters(rdg_pass* pass);

    void add_mesh(rdg_pass* pass, const rdg_mesh& mesh);
    const std::vector<rdg_mesh>& get_meshes(rdg_pass* pass) const;

    void add_dispatch(rdg_pass* pass, const rdg_dispatch& dispatch);
    const std::vector<rdg_dispatch>& get_dispatches(rdg_pass* pass) const;

    void reset();

private:
    struct resource_slot
    {
        rhi_texture* texture;
        rhi_buffer* buffer;
    };

    struct pass_slot
    {
        std::vector<rdg_mesh> meshes;
        std::vector<rdg_dispatch> dispatches;
        std::vector<rhi_ptr<rhi_parameter>> parameters;
    };

    std::vector<resource_slot> m_resource_slots;
    std::vector<pass_slot> m_pass_slots;

    rhi_parameter* m_camera;
    rhi_parameter* m_light;
};
} // namespace violet