#pragma once

#include "core/engine.hpp"
#include "graphics/morph_target.hpp"
#include "graphics/render_graph/rdg_allocator.hpp"

namespace violet
{
class skinning_system : public system
{
public:
    skinning_system();

    bool initialize(const dictionary& config) override;

    void update();

    bool need_record() const noexcept
    {
        return !m_morphing_queue.empty() || !m_skinning_queue.empty();
    }

    void morphing(rhi_command* command, rdg_allocator* allocator);
    void skinning(rhi_command* command, rdg_allocator* allocator);

private:
    struct morphing_data
    {
        morph_target_buffer* morph_target_buffer;
        const float* weights;
        std::size_t weight_count;

        rhi_buffer* morph_vertex_buffer;
    };

    struct skinning_data
    {
        rhi_shader* shader;
        std::size_t vertex_count;

        rhi_buffer* skeleton;
        std::vector<rhi_buffer*> input;
        std::vector<rhi_buffer*> output;
    };

    void update_skin();
    void update_skeleton();
    void update_morph();

    std::vector<morphing_data> m_morphing_queue;
    std::vector<skinning_data> m_skinning_queue;

    std::uint32_t m_system_version{0};
};
} // namespace violet