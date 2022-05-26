#pragma once

#include "element_extent.hpp"
#include "graphics_interface.hpp"
#include "math/math.hpp"
#include <vector>

namespace ash::ui
{
enum element_control_type
{
    ELEMENT_CONTROL_TYPE_CONTAINER,
    ELEMENT_CONTROL_TYPE_BLOCK,
    ELEMENT_CONTROL_TYPE_TEXT,
    ELEMENT_CONTROL_TYPE_IMAGE
};

struct element_mesh
{
    void reset()
    {
        vertex_position.clear();
        vertex_uv.clear();
        vertex_color.clear();
        indices.clear();

        texture = nullptr;
    }

    std::vector<math::float3> vertex_position;
    std::vector<math::float2> vertex_uv;
    std::vector<std::uint32_t> vertex_color;
    std::vector<std::uint32_t> indices;

    graphics::resource* texture;
};

class element_control
{
public:
    element_control() : m_type(ELEMENT_CONTROL_TYPE_BLOCK), m_layer(0), m_dirty(true) {}
    virtual ~element_control() = default;

    virtual void extent(const element_extent& extent) = 0;
    void layer(std::uint32_t layer) noexcept { m_layer = layer; }
    std::uint32_t layer() const noexcept { return m_layer; }

    element_control_type type() const noexcept { return m_type; }
    const element_mesh& mesh() const noexcept { return m_mesh; }

    bool dirty() const noexcept { return m_dirty; }
    void reset_dirty() noexcept { m_dirty = false; }

protected:
    float depth() const noexcept { return 1.0f - static_cast<float>(m_layer) * 0.01f; }

    element_control_type m_type;
    element_mesh m_mesh;

    bool m_dirty;

private:
    std::uint32_t m_layer;
};
} // namespace ash::ui