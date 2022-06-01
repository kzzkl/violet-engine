#include "ui/controls/view.hpp"

namespace ash::ui
{
view::view()
{
}

void view::render(renderer& renderer)
{
    renderer.scissor_push(layout_extent());
    element::render(renderer);
    renderer.scissor_pop();
}
} // namespace ash::ui