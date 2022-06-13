#include "ui/controls/image.hpp"

namespace ash::ui
{
image::image(graphics::resource* texture)
{
    m_mesh.vertex_position = {
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}
    };
    m_mesh.vertex_uv = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };
    m_mesh.vertex_color = {0, 0, 0, 0};
    m_mesh.indices = {0, 1, 2, 0, 2, 3};
    m_mesh.texture = texture;

    if (texture != nullptr)
    {
        auto texture_extent = texture->extent();
        width(texture_extent.width);
        height(texture_extent.height);
    }
}

void image::texture(graphics::resource* texture, bool resize)
{
    if (resize)
    {
        auto texture_extent = texture->extent();
        width(texture_extent.width);
        height(texture_extent.height);
    }
    m_mesh.texture = texture;

    mark_dirty();
}

void image::render(renderer& renderer)
{
    renderer.draw(RENDER_TYPE_IMAGE, m_mesh);
    element::render(renderer);
}

void image::on_extent_change(const element_extent& extent)
{
    float z = depth();
    m_mesh.vertex_position = {
        {extent.x,                extent.y,                 z},
        {extent.x + extent.width, extent.y,                 z},
        {extent.x + extent.width, extent.y + extent.height, z},
        {extent.x,                extent.y + extent.height, z}
    };
}
} // namespace ash::ui