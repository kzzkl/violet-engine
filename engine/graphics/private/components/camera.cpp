#include "components/camera.hpp"
#include "common/hash.hpp"

namespace violet
{
camera::camera(renderer* renderer)
{
    m_perspective.fov = 45.0f;
    m_perspective.near_z = 0.1f;
    m_perspective.far_z = 1000.0f;

    m_parameter_data.view = matrix::identity();
    m_parameter_data.projection = matrix::perspective(
        m_perspective.fov,
        static_cast<float>(m_viewport.width) / static_cast<float>(m_viewport.height),
        m_perspective.near_z,
        m_perspective.far_z);

    m_parameter = renderer->create_parameter(renderer->get_parameter_layout("violet camera"));
}

camera::~camera()
{
}

void camera::set_perspective(float fov, float near_z, float far_z)
{
    m_perspective.fov = fov;
    m_perspective.near_z = near_z;
    m_perspective.far_z = far_z;
    update_projection();
}

void camera::set_position(const float3& position)
{
    m_parameter_data.positon = position;
    update_parameter();
}

void camera::set_view(const float4x4& view)
{
    m_parameter_data.view = view;

    float4x4_simd v = simd::load(m_parameter_data.view);
    float4x4_simd p = simd::load(m_parameter_data.projection);
    simd::store(matrix_simd::mul(v, p), m_parameter_data.view_projection);

    update_parameter();
}

void camera::set_skybox(rhi_texture* texture, rhi_sampler* sampler)
{
    m_parameter->set_texture(1, texture, sampler);
}

void camera::resize(std::uint32_t width, std::uint32_t height)
{
    m_scissor.max_x = m_scissor.min_x + width;
    m_scissor.max_y = m_scissor.min_y + height;

    m_viewport.width = width;
    m_viewport.height = height;
    m_viewport.min_depth = 0.0f;
    m_viewport.max_depth = 1.0f;

    update_projection();
}

void camera::update_projection()
{
    m_parameter_data.projection = matrix::perspective(
        m_perspective.fov,
        static_cast<float>(m_viewport.width) / static_cast<float>(m_viewport.height),
        m_perspective.near_z,
        m_perspective.far_z);

    float4x4_simd v = simd::load(m_parameter_data.view);
    float4x4_simd p = simd::load(m_parameter_data.projection);
    simd::store(matrix_simd::mul(v, p), m_parameter_data.view_projection);

    update_parameter();
}

void camera::update_parameter()
{
    m_parameter->set_uniform(0, &m_parameter_data, sizeof(camera_parameter), 0);
}
} // namespace violet