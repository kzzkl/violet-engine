#pragma once

#include "component.hpp"
#include "math.hpp"

namespace ash::graphics
{
struct main_camera
{
};

struct camera
{
public:
    void set(float fov, float aspect, float near_z, float far_z)
    {
        math::float4x4_simd proj = math::matrix_simd::perspective(fov, aspect, near_z, far_z);
        math::simd::store(proj, m_perspective);
    }

    inline const math::float4x4 get_perspective() const noexcept { return m_perspective; }

private:
    math::float4x4 m_perspective;
};
} // namespace ash::graphics

namespace ash::ecs
{
template <>
struct component_trait<ash::graphics::main_camera>
{
    static constexpr std::size_t id = uuid("532f50db-21cd-46a5-9e05-abe5f471f11f").hash();
};

template <>
struct component_trait<ash::graphics::camera>
{
    static constexpr std::size_t id = uuid("79a9eb68-3fae-4f9a-a75f-9785c4823686").hash();
};
} // namespace ash::ecs