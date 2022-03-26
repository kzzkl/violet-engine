#pragma once

#include "assert.hpp"
#include "component.hpp"

namespace ash::graphics
{
class render_pipeline;
struct visual
{
    template <typename T, std::size_t I, typename D>
    void set(const D& data)
    {
        T* p = dynamic_cast<T*>(material);
        if (p)
            p->set<I>(data);
        else
            ASH_ASSERT(false);
    }

    render_pipeline* group;

    std::unique_ptr<render_parameter> object;
    std::unique_ptr<render_parameter> material;
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