#include "ui/font.hpp"
#include "graphics/graphics.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <fstream>

namespace ash::ui
{
font::font(std::string_view font, std::size_t size)
{
    FT_Error error;

    FT_Library library;
    error = FT_Init_FreeType(&library);
    if (error)
        throw std::runtime_error("FT_Init_FreeType failed.");

    FT_Face face;
    error = FT_New_Face(library, font.data(), 0, &face);
    if (error)
        throw std::runtime_error("FT_New_Face failed.");

    error = FT_Set_Char_Size(face, size * 64, 0, 96, 0);
    if (error)
        throw std::runtime_error("FT_Set_Char_Size failed.");

    int max_dim = (1 + (face->size->metrics.height >> 6)) * ceilf(sqrtf(128));
    int tex_width = 1;
    while (tex_width < max_dim)
        tex_width <<= 1;
    int tex_height = tex_width;

    std::vector<std::uint8_t> pixels(tex_width * tex_height);

    auto draw_bitmap = [&pixels, tex_width, tex_height](FT_Bitmap* bitmap, FT_Int x, FT_Int y) {
        FT_Int x_max = x + bitmap->width;
        FT_Int y_max = y + bitmap->rows;

        for (FT_Int i = x, p = 0; i < x_max; i++, p++)
        {
            for (FT_Int j = y, q = 0; j < y_max; j++, q++)
            {
                if (i < 0 || j < 0 || i >= tex_width || j >= tex_height)
                    continue;

                pixels[j * tex_width + i] |= bitmap->buffer[q * bitmap->width + p];
            }
        }
    };

    m_heigth = face->size->metrics.height >> 6;
    FT_Vector pen = {0, static_cast<FT_Pos>(m_heigth)};
    for (std::size_t i = 0; i < m_glyph.size(); ++i)
    {
        FT_Set_Transform(face, nullptr, &pen);

        error = FT_Load_Char(face, i, FT_LOAD_RENDER);
        if (error)
            continue;

        FT_GlyphSlot slot = face->glyph;

        if (slot->bitmap_left + slot->bitmap.width >= tex_width)
        {
            pen.x = 0;
            pen.y += m_heigth;

            --i;
            continue;
        }

        FT_Pos x_min = slot->bitmap_left;
        FT_Pos x_max = x_min + slot->bitmap.width;
        FT_Pos y_min = pen.y - slot->bitmap_top;
        FT_Pos y_max = y_min + slot->bitmap.rows;

        draw_bitmap(&slot->bitmap, x_min, y_min);

        m_glyph[i].width = slot->bitmap.width;
        m_glyph[i].height = slot->bitmap.rows;
        m_glyph[i].bearing_x = face->glyph->metrics.horiBearingX >> 6;
        m_glyph[i].bearing_y = face->glyph->metrics.horiBearingY >> 6;
        m_glyph[i].advance = face->glyph->advance.x >> 6;
        m_glyph[i].uv1 = {
            static_cast<float>(x_min) / static_cast<float>(tex_width),
            static_cast<float>(y_min) / static_cast<float>(tex_height)};
        m_glyph[i].uv2 = {
            static_cast<float>(x_max) / static_cast<float>(tex_width),
            static_cast<float>(y_max) / static_cast<float>(tex_height)};

        pen.x += slot->advance.x;
        pen.y += slot->advance.y;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    m_texture = system<graphics::graphics>().make_texture(
        pixels.data(),
        tex_width,
        tex_height,
        graphics::resource_format::R8_UNORM);
}
} // namespace ash::ui