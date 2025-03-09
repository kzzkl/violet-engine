#pragma once

#include "core/engine.hpp"
#include "graphics/morph_target.hpp"

namespace violet
{
class geometry;
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

    void morphing(rhi_command* command);
    void skinning(rhi_command* command);

private:
    struct morphing_data
    {
        morph_target_buffer* morph_target_buffer;
        const float* weights;
        std::size_t weight_count;

        raw_buffer* morph_vertex_buffer;
    };

    struct skinning_data
    {
        rhi_shader* shader;
        std::size_t vertex_count;

        structured_buffer* skeleton;

        geometry* original_geometry;
        geometry* skinned_geometry;

        std::vector<raw_buffer*> additional_buffers;
    };

    void update_skin();
    void update_skeleton();
    void update_morph();

    std::vector<morphing_data> m_morphing_queue;
    std::vector<skinning_data> m_skinning_queue;

    std::uint32_t m_system_version{0};
};
} // namespace violet