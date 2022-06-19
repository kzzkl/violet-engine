#pragma once

#include "page.hpp"
#include "ui/controls/input.hpp"
#include "ui/controls/label.hpp"

namespace ash::sample
{
class input_page : public page
{
public:
    input_page();

private:
    void initialize_sample_input();

    std::unique_ptr<ui::input> m_input;
    std::unique_ptr<ui::label> m_input_text;
};
} // namespace ash::sample