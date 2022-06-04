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

    auto texture_extent = texture->extent();
    width(texture_extent.width);
    height(texture_extent.height);
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

void image::on_extent_change()
{
    auto& e = extent();
    float z = depth();
    m_mesh.vertex_position = {
        {e.x,           e.y,            z},
        {e.x + e.width, e.y,            z},
        {e.x + e.width, e.y + e.height, z},
        {e.x,           e.y + e.height, z}
    };
}
} // namespace ash::ui