#pragma once

#include "ui/color.hpp"
#include "ui/control.hpp"

namespace violet
{
class font;

struct label_theme
{
    const font* text_font;
    std::uint32_t text_color;
};

class label : public control
{
public:
    label();
    label(std::string_view content, const label_theme& theme);

    void text(std::string_view content);
    void text_color(std::uint32_t color);

    std::string text() const noexcept { return m_text; }

    virtual const control_mesh* mesh() const noexcept override { return &m_mesh; }

private:
    std::string m_text;
    const font* m_font;

    std::vector<float2> m_position;
    std::vector<float2> m_uv;
    std::vector<std::uint32_t> m_color;
    std::vector<std::uint32_t> m_indices;

    control_mesh m_mesh;
};
} // namespace violet