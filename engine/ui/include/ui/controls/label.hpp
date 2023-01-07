#pragma once

#include "ui/color.hpp"
#include "ui/element.hpp"

namespace violet::ui
{
class font;

struct label_theme
{
    const font* text_font;
    std::uint32_t text_color;
};

class label : public element
{
public:
    label();
    label(std::string_view content, const label_theme& theme);

    void text(std::string_view content);
    void text_color(std::uint32_t color);

    std::string text() const noexcept { return m_text; }

    virtual const element_mesh* mesh() const noexcept override { return &m_mesh; }

private:
    std::string m_text;
    const font* m_font;

    std::vector<math::float2> m_position;
    std::vector<math::float2> m_uv;
    std::vector<std::uint32_t> m_color;
    std::vector<std::uint32_t> m_indices;

    element_mesh m_mesh;
};
} // namespace violet::ui