#pragma once

#include "page.hpp"
#include "ui/controls/image.hpp"

namespace ash::sample
{
class image_page : public page
{
public:
    image_page();

private:
    void initialize_sample_image();

    std::unique_ptr<ui::image> m_file_image;
    std::unique_ptr<graphics::resource> m_cat_image;
};
} // namespace ash::sample