#include "image_page.hpp"
#include "graphics/graphics.hpp"

namespace ash::sample
{
image_page::image_page()
{
    m_title = std::make_unique<text_title_1>("Image");
    m_title->link(this);

    m_description = std::make_unique<text_content>("You can use image control to display image.");
    m_description->link(this);

    for (std::size_t i = 0; i < 2; ++i)
    {
        auto display = std::make_unique<display_panel>();
        display->flex_direction(ui::LAYOUT_FLEX_DIRECTION_ROW);
        display->align_items(ui::LAYOUT_ALIGN_CENTER);
        m_display.push_back(std::move(display));
    }
    initialize_sample_image();
}

void image_page::initialize_sample_image()
{
    // Image file.
    m_file_image_title = std::make_unique<text_title_2>("Image loaded from file");
    m_file_image_title->link(this);
    m_display[0]->link(this);

    m_cat_image = system<graphics::graphics>().make_texture("resource/image/huhu.jpg");
    m_file_image = std::make_unique<ui::image>(m_cat_image.get());
    m_file_image->link(m_display[0].get());
}
} // namespace ash::sample