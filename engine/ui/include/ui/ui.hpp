#pragma once

#include "core/context.hpp"
#include "element_tree.hpp"
#include "font.hpp"
#include "graphics/graphics.hpp"
#include "ui_pipeline.hpp"

namespace ash::ui
{
static const char* DEFAULT_TEXT_FONT = "NotoSans-Regular";
static const char* DEFAULT_ICON_FONT = "remixicon";

class ui : public core::system_base
{
public:
    using font_type = font;

public:
    ui();

    virtual bool initialize(const dictionary& config) override;

    void tick();

    void load_font(std::string_view name, std::string_view ttf_file, std::size_t size);
    font_type& font(std::string_view name);

    element* root() const noexcept { return m_tree.get(); }

private:
    void resize(std::uint32_t width, std::uint32_t height);

    graphics::pipeline_parameter* allocate_material_parameter();

    std::vector<std::unique_ptr<graphics::resource>> m_vertex_buffers;
    std::unique_ptr<graphics::resource> m_index_buffer;

    std::unique_ptr<graphics::pipeline_parameter> m_mvp_parameter;

    std::size_t m_material_parameter_counter;
    std::vector<std::unique_ptr<graphics::pipeline_parameter>> m_material_parameter_pool;

    std::unique_ptr<ui_pipeline> m_pipeline;

    std::unordered_map<std::string, std::unique_ptr<font_type>> m_fonts;

    ecs::entity m_entity;
    std::unique_ptr<element_tree> m_tree;

    renderer m_renderer;
};
}; // namespace ash::ui