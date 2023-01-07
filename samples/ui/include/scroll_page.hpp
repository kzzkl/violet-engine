#pragma once

#include "page.hpp"
#include "ui/controls/scroll_view.hpp"

namespace violet::sample
{
class scroll_page : public page
{
public:
    scroll_page();

private:
    void initialize_sample_scroll();

    std::unique_ptr<ui::scroll_view> m_scroll_view;
    std::vector<std::unique_ptr<ui::panel>> m_panels;
};
} // namespace violet::sample