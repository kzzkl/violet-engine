#pragma once

#include <string>

namespace ash::sample
{
struct text_style
{
    const char* font;
    const char* font_path;
    std::size_t font_size;

    float margin_top;
    float margin_bottom;
};

struct style
{
    static constexpr text_style title_1 = {
        .font = "title 1",
        .font_path = "engine/font/NotoSans-SemiBold.ttf",
        .font_size = 25,
        .margin_top = 20,
        .margin_bottom = 10};
    static constexpr text_style title_2 = {
        .font = "title 2",
        .font_path = "engine/font/NotoSans-SemiBold.ttf",
        .font_size = 20,
        .margin_top = 10,
        .margin_bottom = 5};
    static constexpr text_style content = {
        .font = "content",
        .font_path = "engine/font/NotoSans-Regular.ttf",
        .font_size = 13,
        .margin_top = 5,
        .margin_bottom = 5};

    static constexpr std::uint32_t background_color = 0xffdfdfe1;
    static constexpr std::uint32_t page_color = 0xfffafafa;
    static constexpr std::uint32_t display_color = 0xffdfdfe1;
};
} // namespace ash::sample