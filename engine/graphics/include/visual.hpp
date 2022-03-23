#pragma once

#include "assert.hpp"
#include "component.hpp"

namespace ash::graphics
{
class render_group;
struct visual
{
    template <typename T, std::size_t I, typename D>
    void set(std::size_t index, const D& data)
    {
        T* p = dynamic_cast<T*>(parameters[index]);
        if (p)
            p->set<I>(data);
        else
            ASH_ASSERT(false);
    }

    render_group* group;

    render_parameter_object* object;
    std::vector<render_parameter_base*> parameters;
};
} // namespace ash::graphics

namespace ash::ecs
{
template <>
struct component_trait<ash::graphics::visual>
{
    static constexpr std::size_t id = ash::uuid("be64e8c8-568e-4fdc-9d8c-d891c8ce2de0").hash();
};
} // namespace ash::ecs