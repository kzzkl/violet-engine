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
    void initialize_input_text();
    void initialize_input_float();

    std::unique_ptr<ui::input> m_input_text;
    std::unique_ptr<ui::label> m_input_text_result;

    std::unique_ptr<ui::input_float> m_input_float;
    std::unique_ptr<ui::label> m_input_float_result;
};
} // namespace ash::sample