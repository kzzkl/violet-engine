#include "ui/font.hpp"
#include "graphics/graphics.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

namespace ash::ui
{
font::font(std::string_view font, std::size_t size) : m_size(size)
{
    auto& graphics = system<graphics::graphics>();

    FT_Library library;

    FT_Error error = FT_Init_FreeType(&library);
    if (error)
    {
        log::error("FT_Init_FreeType failed.");
        return;
    }

    FT_Face face;
    error = FT_New_Face(library, font.data(), 0, &face);
    if (error)
    {
        log::error("FT_New_Face failed.");
        return;
    }

    error = FT_Set_Char_Size(face, size * 64, 0, 96, 0);
    if (error)
    {
        log::error("FT_Set_Char_Size failed.");
        return;
    }

    int max_dim = (1 + (face->size->metrics.height >> 6)) * ceilf(sqrtf(128));
    int tex_width = 1;
    while (tex_width < max_dim)
        tex_width <<= 1;
    int tex_height = tex_width;

    std::vector<std::uint8_t> pixels(tex_width * tex_height);

    int pen_x = 0, pen_y = 0;
    for (std::size_t i = 0; i < m_glyph.size(); ++i)
    {
        FT_Load_Char(face, i, FT_LOAD_RENDER | FT_LOAD_DEFAULT);
        FT_Bitmap* bmp = &face->glyph->bitmap;

        if (pen_x + bmp->width >= tex_width)
        {
            pen_x = 0;
            pen_y += ((face->size->metrics.height >> 6) + 1);
        }

        for (int row = 0; row < bmp->rows; ++row)
        {
            for (int col = 0; col < bmp->width; ++col)
            {
                int x = pen_x + col;
                int y = pen_y + row;
                pixels[y * tex_width + x] = bmp->buffer[row * bmp->pitch + col];
            }
        }

        m_glyph[i].width = bmp->width;
        m_glyph[i].height = bmp->rows;
        m_glyph[i].bearing_x = face->glyph->bitmap_left;
        m_glyph[i].bearing_y = face->glyph->bitmap_top;
        m_glyph[i].advance = face->glyph->advance.x >> 6;
        m_glyph[i].uv1 = {
            static_cast<float>(pen_x) / static_cast<float>(tex_width),
            static_cast<float>(pen_y) / static_cast<float>(tex_height)};
        m_glyph[i].uv2 = {
            static_cast<float>(pen_x + bmp->width) / static_cast<float>(tex_width),
            static_cast<float>(pen_y + bmp->rows) / static_cast<float>(tex_height)};

        pen_x += bmp->width + 1;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    m_texture = graphics.make_texture(
        pixels.data(),
        tex_width,
        tex_height,
        graphics::resource_format::R8_UNORM);
}
} // namespace ash::ui