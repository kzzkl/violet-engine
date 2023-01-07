#pragma once

#include "core/context.hpp"
#include "ecs/entity.hpp"
#include "ui/element.hpp"
#include "ui/font.hpp"
#include "ui/renderer.hpp"
#include "ui/theme.hpp"

namespace violet::ui
{
class element_tree;
class mvp_pipeline_parameter;
class offset_pipeline_parameter;
class material_pipeline_parameter;
class ui_pipeline;
class ui : public core::system_base
{
public:
    using font_type = font;

public:
    ui();

    virtual bool initialize(const dictionary& config) override;

    void tick();

    void load_font(std::string_view name, std::string_view ttf_file, std::size_t size);
    const font_type* font(std::string_view name);

    const font_type* default_text_font();
    const font_type* default_icon_font();

    element* root() const noexcept;

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
    void resize(std::uint32_t width, std::uint32_t height);

    material_pipeline_parameter* allocate_material_parameter();

    std::vector<std::unique_ptr<graphics::resource_interface>> m_vertex_buffers;
    std::unique_ptr<graphics::resource_interface> m_index_buffer;

    std::size_t m_material_parameter_counter;
    std::vector<std::unique_ptr<material_pipeline_parameter>> m_material_parameter_pool;

    std::unique_ptr<ui_pipeline> m_pipeline;

    std::unordered_map<std::string, std::unique_ptr<font_type>> m_fonts;
    theme_manager m_theme_manager;

    ecs::entity m_entity;
    std::unique_ptr<element_tree> m_tree;

    renderer m_renderer;
};
}; // namespace violet::ui