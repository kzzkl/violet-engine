#pragma once

#include "ui/controls/label.hpp"
#include "ui/controls/panel.hpp"

namespace ash::sample
{
class text_title_1 : public ui::label
{
public:
    text_title_1(std::string_view content, std::uint32_t color = ui::COLOR_BLACK);
};

class text_title_2 : public ui::label
{
public:
    text_title_2(std::string_view content, std::uint32_t color = ui::COLOR_BLACK);
};

class text_content : public ui::label
{
public:
    text_content(std::string_view content, std::uint32_t color = ui::COLOR_BLACK);
};

class page : public ui::element
{
public:
    page();
};

class display_panel : public ui::panel
{
public:
    display_panel();
};
} // namespace ash::sample