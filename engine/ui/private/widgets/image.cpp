#include "ui/widgets/image.hpp"

namespace violet
{
image::image(rhi_texture* texture)
{
    m_position = {};
    m_uv = {
        float2{0.0f, 0.0f},
        float2{1.0f, 0.0f},
        float2{1.0f, 1.0f},
        float2{0.0f, 1.0f}
    };
    m_color = {0, 0, 0, 0};
    m_indices = {0, 1, 2, 0, 2, 3};

    m_mesh = {
        .type = ELEMENT_MESH_TYPE_IMAGE,
        .position = m_position.data(),
        .uv = m_uv.data(),
        .color = m_color.data(),
        .vertex_count = 4,
        .indices = m_indices.data(),
        .index_count = 6,
        .scissor = false,
        .texture = texture};

    if (texture != nullptr)
    {
        auto texture_extent = texture->get_extent();
        layout()->set_width(texture_extent.width);
        layout()->set_height(texture_extent.height);
    }
}

void image::texture(rhi_texture* texture, bool resize)
{
    if (resize)
    {
        auto texture_extent = texture->get_extent();
        layout()->set_width(texture_extent.width);
        layout()->set_height(texture_extent.height);
    }
    m_mesh.texture = texture;

    mark_dirty();
}

void image::on_extent_change(float width, float height)
{
    m_position[1] = {width, 0.0f};
    m_position[2] = {width, height};
    m_position[3] = {0.0f, height};
}
} // namespace violet