#pragma once

#include "graphics/render_graph/rdg_command.hpp"
#include "graphics/shader.hpp"

namespace violet
{
using shading_gbuffer = std::uint8_t;
enum buildin_shading_gbuffer : shading_gbuffer
{
    SHADING_GBUFFER_ALBEDO,   // R8G8B8A8_UNORM
    SHADING_GBUFFER_MATERIAL, // R8G8B8_UNORM, x: Roughness, y: Metallic
    SHADING_GBUFFER_NORMAL,   // R32_UINT, 24 bit: Octahedron Normal, 8 bit: Shading Model
    SHADING_GBUFFER_EMISSIVE, // R8G8B8A8_UNORM
    SHADING_GBUFFER_CUSTOM,
};

using shading_auxiliary_buffer = std::uint8_t;
enum buildin_shading_auxiliary_buffer : shading_auxiliary_buffer
{
    SHADING_AUXILIARY_BUFFER_DEPTH,
    SHADING_AUXILIARY_BUFFER_AO,
    SHADING_AUXILIARY_BUFFER_CUSTOM,
};

class shading_model_base
{
public:
    shading_model_base(
        std::string_view name,
        const std::vector<shading_gbuffer>& required_gbuffers,
        const std::vector<shading_auxiliary_buffer>& required_auxiliary_buffers)
        : m_name(name),
          m_required_gbuffers(required_gbuffers),
          m_required_auxiliary_buffers(required_auxiliary_buffers)
    {
        assert(m_required_gbuffers.size() + m_required_auxiliary_buffers.size() <= 8);
    }

    virtual void bind(std::span<const rdg_texture_srv> auxiliary_buffers, rdg_command& command) = 0;

    const std::vector<shading_gbuffer>& get_required_gbuffers() const noexcept
    {
        return m_required_gbuffers;
    }

    const std::vector<shading_auxiliary_buffer>& get_required_auxiliary_buffers() const noexcept
    {
        return m_required_auxiliary_buffers;
    }

    const std::string& get_name() const noexcept
    {
        return m_name;
    }

private:
    std::string m_name;

    std::vector<shading_gbuffer> m_required_gbuffers;
    std::vector<shading_auxiliary_buffer> m_required_auxiliary_buffers;
};

template <typename T>
concept has_extra_shading_constant = requires(T t) {
    t.get_constant();
} && (!std::is_void_v<decltype(std::declval<T>().get_constant())>);

template <typename T>
class shading_model : public shading_model_base
{
public:
    virtual ~shading_model() = default;

    void bind(std::span<const rdg_texture_srv> auxiliary_buffers, rdg_command& command) final
    {
        m_auxiliary_buffers = auxiliary_buffers;

        auto* derived = static_cast<T*>(this);
        command.set_pipeline(derived->get_pipeline());

        if constexpr (has_extra_shading_constant<T>)
        {
            auto constant = derived->get_constant();
            command.set_constant(constant, sizeof(shading_model_cs::constant_data));
        }
    }

protected:
    bool has_auxiliary_buffer(shading_auxiliary_buffer resource) const noexcept
    {
        return m_auxiliary_buffers[resource];
    }

private:
    shading_model(
        std::string_view name,
        const std::vector<shading_gbuffer>& gbuffers,
        const std::vector<shading_auxiliary_buffer>& auxiliary_buffers = {})
        : shading_model_base(name, gbuffers, auxiliary_buffers)
    {
    }

    std::span<const rdg_texture_srv> m_auxiliary_buffers;

    friend T;
};
} // namespace violet