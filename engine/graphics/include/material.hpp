#pragma once

#include "component.hpp"
#include "graphics_interface.hpp"

namespace ash::graphics
{
class material
{
public:
    template <typename T>
    void set()
    {
    }

private:
    pipeline_parameter* m_material;
};
} // namespace ash::graphics

namespace ash::ecs
{
template <>
struct component_trait<ash::graphics::material>
{
    static constexpr std::size_t id = ash::uuid("abbc74ad-d14a-48fb-8070-d8b7715aa5b7").hash();
};
} // namespace ash::ecs