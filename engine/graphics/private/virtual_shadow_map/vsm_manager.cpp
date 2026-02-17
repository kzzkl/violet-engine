#include "virtual_shadow_map/vsm_manager.hpp"
#include "gpu_buffer_uploader.hpp"
#include "math/matrix.hpp"
#include "virtual_shadow_map/vsm_common.hpp"

namespace violet
{
vsm_manager::vsm_manager(bool enable_occlusion)
    : m_vsms(8) // max vsm count is 256
{
    auto& device = render_device::instance();

    m_vsms.set_name("VSM Buffer");

    m_virtual_page_table = device.create_buffer({
        .size = sizeof(std::uint32_t) * MAX_VSM_COUNT * VSM_VIRTUAL_PAGE_TABLE_PAGE_COUNT,
        .flags = RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST,
    });
    device.set_name(m_virtual_page_table.get(), "VSM Virtual Page Table");

    m_physical_page_table = device.create_buffer({
        .size = sizeof(vec4u) * VSM_PHYSICAL_PAGE_TABLE_PAGE_COUNT,
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

    rhi_texture_desc physical_shadow_map_desc = {
        .extent =
            {
                .width = VSM_PHYSICAL_RESOLUTION.x,
                .height = VSM_PHYSICAL_RESOLUTION.y,
            },
        .format = RHI_FORMAT_R32_UINT,
        .flags = RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE,
        .level_count = 1,
        .layer_count = 1,
        .layout = RHI_TEXTURE_LAYOUT_GENERAL,
    };

    m_physical_shadow_map_static = device.create_texture(physical_shadow_map_desc);
    device.set_name(m_physical_shadow_map_static.get(), "VSM Physical Shadow Map Static");

    m_physical_shadow_map_final = device.create_texture(physical_shadow_map_desc);
    device.set_name(m_physical_shadow_map_final.get(), "VSM Physical Shadow Map Final");

    if (enable_occlusion)
    {
        m_hzb = device.create_texture({
            .extent =
                {
                    .width = VSM_PHYSICAL_RESOLUTION.x / 2,
                    .height = VSM_PHYSICAL_RESOLUTION.y / 2,
                },
            .format = RHI_FORMAT_R32_FLOAT,
            .flags = RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE,
            .level_count = static_cast<std::uint32_t>(
                std::log2(VSM_PHYSICAL_RESOLUTION.x) - std::log2(VSM_PHYSICAL_PAGE_TABLE_SIZE_X)),
            .layer_count = 1,
            .layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
        });
        device.set_name(m_hzb.get(), "VSM HZB");
    }
}

render_id vsm_manager::add_vsm(light_type light_type)
{
    render_id vsm_id = m_vsms.add(get_vsm_count(light_type));

    auto& vsm = m_vsms[vsm_id];
    vsm.light_type = light_type;
    m_vsms.mark_dirty(vsm_id);

    switch (light_type)
    {
    case LIGHT_DIRECTIONAL:
        m_directional_lights[vsm_id] = {};
        break;
    default:
        break;
    }

    return vsm_id;
}

void vsm_manager::remove_vsm(render_id vsm_id)
{
    switch (m_vsms[vsm_id].light_type)
    {
    case LIGHT_DIRECTIONAL:
        m_directional_lights.erase(vsm_id);
        break;
    default:
        break;
    }

    m_vsms.remove(vsm_id);
}

void vsm_manager::set_vsm(render_id vsm_id, const vsm_directional_light_data& light)
{
    assert(m_vsms[vsm_id].light_type == LIGHT_DIRECTIONAL);

    vec3f up = {0.0f, 1.0f, 0.0f};
    if (std::abs(vector::dot(up, light.light_direction)) > 0.99f)
    {
        up = {.x = 1.0f, .y = 0.0f, .z = 0.0f};
    }

    mat4f matrix_v = matrix::look_at({}, light.light_direction, up);

    vec3f center = matrix::mul(
        vec4f{light.camera_position.x, light.camera_position.y, light.camera_position.z, 1.0f},
        matrix_v);

    float cascade_radius = 2.56f;

    auto& light_data = m_directional_lights[vsm_id];
    bool force_invalidate = light.light_direction != light_data.light_direction;
    light_data = light;

    std::uint32_t frame = render_device::instance().get_frame_count();

    std::uint32_t cascade_count = get_vsm_count(LIGHT_DIRECTIONAL);
    for (std::uint32_t cascade = 0; cascade < cascade_count; ++cascade, cascade_radius *= 2.0f)
    {
        auto& cascade_vsm = m_vsms[vsm_id + cascade];

        float cascade_page_size = cascade_radius * 2.0f / VSM_VIRTUAL_PAGE_TABLE_SIZE;

        vec2i page_coord = {
            static_cast<std::int32_t>(std::floor(center.x / cascade_page_size)),
            static_cast<std::int32_t>(std::floor(center.y / cascade_page_size)),
        };

        float view_z_radius = cascade_radius * 500.0f;
        bool view_z_dirty = center.z < cascade_vsm.view_z - (view_z_radius * 0.9f) ||
                            center.z > cascade_vsm.view_z + (view_z_radius * 0.9f);

        if (cascade_vsm.page_coord == page_coord && !view_z_dirty && !force_invalidate)
        {
            continue;
        }

        if (view_z_dirty)
        {
            cascade_vsm.view_z = center.z;
            cascade_vsm.cache_epoch = frame;
        }

        if (force_invalidate)
        {
            cascade_vsm.cache_epoch = frame;
        }

        cascade_vsm.matrix_v = matrix_v;
        cascade_vsm.matrix_v[3][0] = -cascade_page_size * static_cast<float>(page_coord.x);
        cascade_vsm.matrix_v[3][1] = -cascade_page_size * static_cast<float>(page_coord.y);

        cascade_vsm.matrix_p = matrix::orthographic(
            cascade_radius * 2.0f,
            cascade_radius * 2.0f,
            cascade_vsm.view_z + view_z_radius,
            cascade_vsm.view_z - view_z_radius);
        cascade_vsm.page_coord = page_coord;
        cascade_vsm.view_z_radius = view_z_radius;
        cascade_vsm.texel_size = cascade_page_size / static_cast<float>(VSM_PAGE_RESOLUTION);

        m_vsms.mark_dirty(vsm_id + cascade);
    }
}

void vsm_manager::update(gpu_buffer_uploader* uploader)
{
    m_vsms.update(
        [](const gpu_vsm& vsm) -> gpu_vsm::gpu_type
        {
            return {
                .page_coord = vsm.page_coord,
                .cache_epoch = vsm.cache_epoch,
                .view_z_radius = vsm.view_z_radius,
                .matrix_v = vsm.matrix_v,
                .matrix_p = vsm.matrix_p,
                .matrix_vp = matrix::mul(vsm.matrix_v, vsm.matrix_p),
                .texel_size = vsm.texel_size,
                .texel_size_inv = 1.0f / vsm.texel_size,
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