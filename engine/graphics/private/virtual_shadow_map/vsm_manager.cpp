#include "virtual_shadow_map/vsm_manager.hpp"
#include "gpu_buffer_uploader.hpp"
#include "math/matrix.hpp"
#include "virtual_shadow_map/vsm_common.hpp"

namespace violet
{
vsm_manager::vsm_manager()
    : m_vsms(8) // max vsm count is 256
{
    auto& device = render_device::instance();

    m_vsms.set_name("VSM Buffer");

    m_virtual_page_table = device.create_buffer({
        .size = sizeof(std::uint32_t) * MAX_VSM_COUNT * VSM_PAGE_COUNT,
        .flags = RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST,
    });
    device.set_name(m_virtual_page_table.get(), "VSM Virtual Page Table");

    struct vsm_physical_page
    {
        vec2i virtual_page_coord;
        std::uint32_t vsm_id;
        std::uint32_t flags;
    };

    m_physical_page_table = device.create_buffer({
        .size = sizeof(vsm_physical_page) * VSM_PHYSICAL_PAGE_COUNT,
        .flags = RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST,
    });
    device.set_name(m_physical_page_table.get(), "VSM Physical Page Table");

    rhi_command* command = device.allocate_command();

    rhi_buffer_region region = {};

    region.size = m_virtual_page_table->get_size();
    command->fill_buffer(m_virtual_page_table.get(), region, 0);

    region.size = m_physical_page_table->get_size();
    command->fill_buffer(m_physical_page_table.get(), region, 0);

    device.execute(command, true);
}

render_id vsm_manager::add_vsm(light_type light_type, render_id light_id, render_id camera_id)
{
    assert(light_type != LIGHT_DIRECTIONAL || camera_id != INVALID_RENDER_ID);

    render_id vsm_id = m_vsms.add(get_vsm_count(light_type));

    auto& vsm = m_vsms[vsm_id];
    vsm.light_type = light_type;
    vsm.light_id = light_id;
    vsm.camera_id = camera_id;

    m_vsms.mark_dirty(vsm_id);

    return vsm_id;
}

void vsm_manager::remove_vsm(render_id vsm_id)
{
    m_vsms.remove(vsm_id);
}

void vsm_manager::set_vsm_light(
    render_id vsm_id,
    const vec3f& light_position,
    const vec3f& light_direction)
{
    auto& vsm = m_vsms[vsm_id];

    if (vsm.light_type == LIGHT_DIRECTIONAL)
    {
        std::uint32_t cascade_count = get_vsm_count(LIGHT_DIRECTIONAL);
        for (std::uint32_t cascade = 0; cascade < cascade_count; ++cascade)
        {
            auto& cascade_vsm = m_vsms[vsm_id + cascade];
            cascade_vsm.light_position = light_position;
            cascade_vsm.light_direction = light_direction;
        }
    }

    vsm.dirty = true;
    m_dirty_vsms.push_back(vsm_id);
}

void vsm_manager::set_vsm_camera(render_id vsm_id, const vec3f& camera_position)
{
    auto& vsm = m_vsms[vsm_id];

    if (vsm.light_type == LIGHT_DIRECTIONAL)
    {
        std::uint32_t cascade_count = get_vsm_count(LIGHT_DIRECTIONAL);
        for (std::uint32_t cascade = 0; cascade < cascade_count; ++cascade)
        {
            auto& cascade_vsm = m_vsms[vsm_id + cascade];
            cascade_vsm.camera_position = camera_position;
        }
    }

    vsm.dirty = true;
    m_dirty_vsms.push_back(vsm_id);
}

void vsm_manager::update(gpu_buffer_uploader* uploader)
{
    for (render_id vsm_id : m_dirty_vsms)
    {
        auto& vsm = m_vsms[vsm_id];

        if (vsm.light_type == LIGHT_DIRECTIONAL)
        {
            vec3f up = {0.0f, 1.0f, 0.0f};
            if (std::abs(vector::dot(up, vsm.light_direction)) > 0.99f)
            {
                up = {.x = 1.0f, .y = 0.0f, .z = 0.0f};
            }

            mat4f matrix_v = matrix::look_at(vec3f{0.0f, 0.0f, 0.0f}, vsm.light_direction, up);
            mat4f matrix_v_inv = matrix::inverse(matrix_v);

            vec3f center = matrix::mul(
                vec4f{vsm.camera_position.x, vsm.camera_position.y, vsm.camera_position.z, 1.0f},
                matrix_v);

            float cascade_page_size = 1.28f * 2.0f / VSM_PAGE_SIZE;

            std::uint32_t cascade_count = get_vsm_count(LIGHT_DIRECTIONAL);
            for (std::uint32_t cascade = 0; cascade < cascade_count; ++cascade)
            {
                vec2i page_coord = {
                    static_cast<std::int32_t>(std::floor(center.x / cascade_page_size)),
                    static_cast<std::int32_t>(std::floor(center.y / cascade_page_size)),
                };

                vec3f cascade_center;
                cascade_center.x = cascade_page_size * static_cast<float>(page_coord.x);
                cascade_center.y = cascade_page_size * static_cast<float>(page_coord.y);
                cascade_center.z = center.z;

                cascade_center = matrix::mul(
                    vec4f{cascade_center.x, cascade_center.y, cascade_center.z, 1.0f},
                    matrix_v_inv);

                mat4f cascade_matrix_v =
                    matrix::look_at(cascade_center, cascade_center + vsm.light_direction, up);

                mat4f cascade_matrix_p = matrix::orthographic(
                    cascade_page_size * VSM_PAGE_SIZE,
                    cascade_page_size * VSM_PAGE_SIZE,
                    0.1f,
                    100.0f);

                auto& cascade_vsm = m_vsms[vsm_id + cascade];
                cascade_vsm.page_offset = page_coord - cascade_vsm.page_coord;
                cascade_vsm.page_coord = page_coord;
                cascade_vsm.light_id = vsm.light_id;
                cascade_vsm.camera_id = vsm.camera_id;
                cascade_vsm.matrix_vp = matrix::mul(cascade_matrix_v, cascade_matrix_p);

                m_vsms.mark_dirty(vsm_id + cascade);

                cascade_page_size *= 2.0f;
            }
        }
    }

    m_dirty_vsms.clear();

    m_vsms.update(
        [](const gpu_vsm& vsm) -> gpu_vsm::gpu_type
        {
            return {
                .page_coord = vsm.page_coord,
                .page_offset = vsm.page_offset,
                .matrix_vp = vsm.matrix_vp,
            };
        },
        [&](rhi_buffer* buffer, const void* data, std::size_t size, std::size_t offset)
        {
            uploader->upload(
                buffer,
                data,
                size,
                offset,
                RHI_PIPELINE_STAGE_COMPUTE,
                RHI_ACCESS_SHADER_READ);
        });
}

constexpr std::uint32_t vsm_manager::get_vsm_count(light_type light_type)
{
    switch (light_type)
    {
    case LIGHT_DIRECTIONAL:
        return 16;
    default:
        return 0;
    }
}
} // namespace violet