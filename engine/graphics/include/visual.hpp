#pragma once

#include "render_group.hpp"

namespace ash::graphics
{
struct visual
{
    template <typename T, std::size_t I, typename D>
    void set(const D& data)
    {
        T* p = dynamic_cast<T*>(material.get());
        if (p)
            p->set<I>(data);
        else
            ASH_ASSERT(false);
    }

    render_group* group;

    std::unique_ptr<render_parameter_object> object;
    std::unique_ptr<render_parameter_base> material;

    std::vector<render_parameter_base*> parameter;
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