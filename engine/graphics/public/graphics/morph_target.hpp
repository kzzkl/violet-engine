#pragma once

#include "graphics/render_graph/rdg_allocator.hpp"
#include "math/types.hpp"
#include <vector>

namespace violet
{
struct morphing_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/morphing.hlsl";

    struct morphing_data
    {
        std::uint32_t morph_target_count;
        float precision;
        std::uint32_t header_buffer;
        std::uint32_t element_buffer;
        std::uint32_t morph_vertex_buffer;
        std::uint32_t weight_buffer;
        std::uint32_t padding0;
        std::uint32_t padding1;
    };

    static constexpr parameter parameter = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages = RHI_SHADER_STAGE_COMPUTE,
            .size = sizeof(morphing_data),
        },
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, parameter},
    };
};

struct morphing_position_cs : public shader_cs
{
    struct morphing_data
    {
        std::uint32_t position_buffer;
        std::uint32_t morph_vertex_buffer;
    };

    static constexpr parameter parameter = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages = RHI_PIPELINE_STAGE_COMPUTE,
            .size = sizeof(morphing_data),
        },
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, parameter},
    };
};

struct morph_element
{
    vec3f position;
    std::uint32_t vertex_index;
};

class morph_target_buffer
{
public:
    void add_morph_target(const std::vector<morph_element>& elements);

    void update_morph(
        rhi_command* command,
        rdg_allocator* allocator,
        rhi_buffer* morph_vertex_buffer,
        std::span<const float> weights);

    std::size_t get_morph_target_count() const noexcept
    {
        return m_morph_targets.size();
    }

private:
    class morph_target
    {
    public:
        morph_target(const std::vector<morph_element>& elements);

        const std::vector<morph_element>& get_elements() const noexcept
        {
            return m_elements;
        }

        float get_position_min() const noexcept
        {
            return m_position_min;
        }

    private:
        std::vector<morph_element> m_elements;

        float m_position_min;
    };

    void update_morph_data();

    std::vector<morph_target> m_morph_targets;

    float m_precision{0.0001};
    std::size_t m_max_element_count{0};

    rhi_ptr<rhi_buffer> m_header_buffer;
    rhi_ptr<rhi_buffer> m_element_buffer;

    bool m_dirty{false};
};
} // namespace violet