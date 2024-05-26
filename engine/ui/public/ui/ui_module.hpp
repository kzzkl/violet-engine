#pragma once

#include "core/engine_module.hpp"
#include "ui/control.hpp"
#include "ui/font.hpp"
#include "ui/rendering/ui_pass.hpp"
#include "ui/theme.hpp"
#include "ui/widget.hpp"

namespace violet
{
class ui_renderer;
class ui_module : public engine_module
{
public:
    using font_type = font;

public:
    ui_module();

    virtual bool initialize(const dictionary& config) override;

    void tick();

    void load_font(std::string_view name, std::string_view ttf_file, std::size_t size);
    const font_type* font(std::string_view name);

    const font_type* default_text_font();
    const font_type* default_icon_font();

    control* root() const noexcept;

    template <typename T>
    void register_theme(std::string_view name, const T& theme)
    {
        m_theme_manager.register_theme(name, theme);
    }

    template <typename T>
    const T& theme(std::string_view name)
    {
        return m_theme_manager.theme<T>(name);
    }

private:
    void initialize_default_theme();

    void update_layout(widget* widget, std::uint32_t width, std::uint32_t height);
    void render(widget* widget, ui_pass* pass);

    std::unordered_map<std::string, std::unique_ptr<font_type>> m_fonts;
    theme_manager m_theme_manager;

    std::unique_ptr<ui_renderer> m_renderer;
};
}; // namespace violet