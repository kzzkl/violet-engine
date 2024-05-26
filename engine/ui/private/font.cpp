#include "ui/font.hpp"
#include "graphics/render_interface.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <fstream>

namespace violet
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

    std::vector<FT_ULong> characters;
    FT_UInt index;
    FT_ULong c = FT_Get_First_Char(face, &index);
    while (true)
    {
        characters.push_back(c);

        c = FT_Get_Next_Char(face, c, &index);
        if (!index)
            break;
    }

    int max_dim = (1 + (face->size->metrics.height >> 6)) * ceilf(sqrtf(characters.size()));
    int tex_width = 1;
    while (tex_width < max_dim)
        tex_width <<= 1;
    int tex_height = tex_width;

    std::vector<std::uint8_t> pixels(tex_width * tex_height);

    auto draw_bitmap = [&pixels, tex_width, tex_height](FT_Bitmap* bitmap, FT_Int x, FT_Int y)
    {
        FT_Int x_max = x + bitmap->width;
        FT_Int y_max = y + bitmap->rows;

        for (FT_Int i = x, p = 0; i < x_max; i++, p++)
        {
            for (FT_Int j = y, q = 0; j < y_max; j++, q++)
            {
                if (i < 0 || j < 0 || i >= tex_width || j >= tex_height)
                    continue;

                pixels[j * tex_width + i] |= bitmap->buffer[q * bitmap->width + p];
                // pixels[j * tex_width + i] = 0xFF;
            }
        }
    };

    m_heigth = face->size->metrics.height >> 6;
    FT_Vector pen = {0, static_cast<FT_Pos>(m_heigth)};
    for (FT_ULong character : characters)
    {
        error = FT_Load_Char(face, character, FT_LOAD_RENDER);
        if (error)
            continue;

        FT_GlyphSlot slot = face->glyph;

        FT_Pos x_min = pen.x + slot->bitmap_left;
        FT_Pos x_max = x_min + slot->bitmap.width;
        FT_Pos y_min = pen.y - slot->bitmap_top;
        FT_Pos y_max = y_min + slot->bitmap.rows;

        draw_bitmap(&slot->bitmap, x_min, y_min);

        auto& glyph = m_glyph[character];
        glyph.width = slot->bitmap.width;
        glyph.height = slot->bitmap.rows;
        glyph.bearing_x = slot->bitmap_left;
        glyph.bearing_y = slot->bitmap_top;
        glyph.advance = slot->advance.x >> 6;
        glyph.uv1 = {
            static_cast<float>(x_min) / static_cast<float>(tex_width),
            static_cast<float>(y_min) / static_cast<float>(tex_height)};
        glyph.uv2 = {
            static_cast<float>(x_max) / static_cast<float>(tex_width),
            static_cast<float>(y_max) / static_cast<float>(tex_height)};

        pen.x += (slot->advance.x >> 6) + 1;
        if (pen.x + m_heigth >= tex_width)
        {
            pen.x = 0;
            pen.y += m_heigth;
        }
    }

    FT_Done_Face(face);
    FT_Done_FreeType(library);
    /*
        m_texture = graphics::rhi::make_texture(
            pixels.data(),
            tex_width,
            tex_height,
            graphics::RESOURCE_FORMAT_R8_UNORM);
            */
}

const glyph_data& font::glyph(std::uint32_t character) const
{
    return m_glyph.at(character);
}
} // namespace violet