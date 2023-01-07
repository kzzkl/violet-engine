#pragma once

#include "ui/controls/label.hpp"
#include "ui/controls/panel.hpp"

namespace violet::sample
{
class page : public ui::element
{
public:
    page(std::string_view title);

protected:
    void add_subtitle(std::string_view subtitle);
    void add_description(std::string_view description);

    ui::panel* add_display_panel();

    ui::panel* display_panel(std::size_t index) const { return m_display_panels[index].get(); }

private:
    std::unique_ptr<ui::label> m_title;
    std::vector<std::unique_ptr<ui::label>> m_subtitles;
    std::vector<std::unique_ptr<ui::label>> m_descriptions;
    std::vector<std::unique_ptr<ui::panel>> m_display_panels;
};
} // namespace violet::sample