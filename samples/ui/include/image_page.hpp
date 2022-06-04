#pragma once

#include "gallery_control.hpp"
#include "ui/controls/image.hpp"

namespace ash::sample
{
class image_page : public page
{
public:
    image_page();

private:
    void initialize_sample_image();

    std::unique_ptr<text_title_1> m_title;
    std::unique_ptr<text_content> m_description;
    std::vector<std::unique_ptr<display_panel>> m_display;

    std::unique_ptr<text_title_2> m_file_image_title;
    std::unique_ptr<ui::image> m_file_image;
    std::unique_ptr<graphics::resource> m_cat_image;
};
} // namespace ash::sample