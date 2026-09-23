#include "graphics/render_scene/render_scene_sdf.hpp"
#include "gpu_buffer_uploader.hpp"
#include "graphics/geometry_manager.hpp"
#include "graphics/render_scene/render_scene_camera.hpp"
#include "math/vector.hpp"

namespace violet
{
render_scene_sdf::render_scene_sdf()
    : m_clipmap_levels(7)
{
}

render_id render_scene_sdf::add_mesh()
{
    return m_meshes.add();
}

void render_scene_sdf::set_mesh_distance_field(render_id mesh_sdf_id, render_id distance_field_id)
{
    auto& instance = m_meshes[mesh_sdf_id];
    instance.distance_field_id = distance_field_id;
    m_meshes.mark_dirty(mesh_sdf_id);
}

void render_scene_sdf::set_mesh_matrix(
    render_id mesh_sdf_id,
    const mat4f& matrix_m,
    const vec3f& scale)
{
    auto& instance = m_meshes[mesh_sdf_id];
    instance.matrix_m = matrix_m;
    instance.scale = vector::max(scale);
    m_meshes.mark_dirty(mesh_sdf_id);
}

void render_scene_sdf::remove_mesh(render_id mesh_sdf_id)
{
    m_meshes.remove(mesh_sdf_id);
}

void render_scene_sdf::update(render_scene_context& context, gpu_buffer_uploader& uploader)
{
    deallocate_clipmaps(context);
    allocate_clipmaps(context);
    update_clipmap(context, uploader);

    update_meshes(uploader);
    update_invalidation_regions(uploader);
}

void render_scene_sdf::reset()
{
    for (auto& clipmap : m_clipmaps)
    {
        clipmap.need_clear = false;
    }

    m_invalidation_regions.clear();
}

void render_scene_sdf::deallocate_clipmaps(render_scene_context& context)
{
    auto& camera_module = context.get_module<render_scene_camera>();

    camera_module.each_removed_camera(
        [&](render_id camera_id)
        {
            if (camera_id < m_clipmaps.size())
            {
                auto& clipmap = m_clipmaps[camera_id];
                clipmap.camera_id = INVALID_RENDER_ID;
                clipmap.page_table = nullptr;
                clipmap.page_atlas = nullptr;
            }
        });
}

void render_scene_sdf::allocate_clipmaps(render_scene_context& context)
{
    auto& camera_module = context.get_module<render_scene_camera>();

    camera_module.each_added_camera(
        [&](render_id camera_id)
        {
            if (m_clipmaps.size() <= camera_id)
            {
                m_clipmaps.resize(camera_id + 1);
            }

            auto& clipmap = m_clipmaps[camera_id];
            clipmap.camera_id = camera_id;
            clipmap.need_clear = true;

            clipmap.level_offset = m_clipmap_levels.add(SDF_CLIPMAP_LEVEL_COUNT);
            for (std::uint32_t i = 0; i < SDF_CLIPMAP_LEVEL_COUNT; ++i)
            {
                m_clipmap_levels[i + clipmap.level_offset] = {
                    .coord = std::numeric_limits<std::int32_t>::max(),
                    .level = i,
                };
            }

            auto& device = render_device::instance();

            struct clipmap_state
            {
                std::uint32_t invalidated_grid_count;
                std::uint32_t invalidated_grid_mesh_count;
                std::uint32_t invalidated_page_count;
                std::uint32_t pages_to_allocate_count;
                std::uint32_t pages_to_update_count;
                std::int32_t free_page_atlas_count;
            };
            clipmap.state = device.create_buffer({
                .size = sizeof(clipmap_state),
                .flags = RHI_BUFFER_STORAGE,
            });
            clipmap.state->set_name("SDF Clipmap State");

            clipmap.page_table = device.create_texture({
                .extent =
                    {
                        .width = SDF_CLIPMAP_PAGE_COUNT_PER_AXIS,
                        .height = SDF_CLIPMAP_PAGE_COUNT_PER_AXIS,
                        .depth = SDF_CLIPMAP_PAGE_COUNT_PER_AXIS * SDF_CLIPMAP_LEVEL_COUNT,
                    },
                .format = RHI_FORMAT_R32_UINT,
                .flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_STORAGE,
                .level_count = 1,
                .layer_count = 1,
                .layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
            });
            clipmap.page_table->set_name("SDF Page Table");

            std::uint32_t page_atlas_page_count_z =
                SDF_CLIPMAP_PAGE_COUNT * SDF_CLIPMAP_LEVEL_COUNT /
                SDF_CLIPMAP_PAGE_ATLAS_PAGE_COUNT_X / SDF_CLIPMAP_PAGE_ATLAS_PAGE_COUNT_Y;
            page_atlas_page_count_z = static_cast<std::uint32_t>(std::ceil(
                static_cast<float>(page_atlas_page_count_z) * SDF_CLIPMAP_PAGE_ATLAS_OCCUPANCY));

            clipmap.page_atlas = device.create_texture({
                .extent =
                    {
                        .width = SDF_CLIPMAP_PAGE_ATLAS_WIDTH,
                        .height = SDF_CLIPMAP_PAGE_ATLAS_HEIGHT,
                        .depth = page_atlas_page_count_z * SDF_CLIPMAP_PAGE_RESOLUTION,
                    },
                .format = RHI_FORMAT_R32_FLOAT,
                .flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_STORAGE,
                .level_count = 1,
                .layer_count = 1,
                .layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
            });
            clipmap.page_atlas->set_name("SDF Page Atlas");

            clipmap.page_atlas_capacity = SDF_CLIPMAP_PAGE_ATLAS_PAGE_COUNT_X *
                                          SDF_CLIPMAP_PAGE_ATLAS_PAGE_COUNT_Y *
                                          page_atlas_page_count_z;

            clipmap.free_pages = device.create_buffer({
                .size = clipmap.page_atlas_capacity * sizeof(std::uint32_t),
                .flags = RHI_BUFFER_STORAGE,
            });
            clipmap.free_pages->set_name("SDF Free Pages");
        });
}

void render_scene_sdf::update_clipmap(render_scene_context& context, gpu_buffer_uploader& uploader)
{
    auto& camera_module = context.get_module<render_scene_camera>();

    constexpr std::int32_t page_size = SDF_CLIPMAP_PAGE_COUNT_PER_AXIS;
    constexpr std::int32_t half_page_size = page_size / 2;

    auto add_invalidation_box = [&](const vec3f& min, const vec3f& max)
    {
        render_id region_id = m_invalidation_regions.add();
        auto& region = m_invalidation_regions[region_id];
        region.bounding_box.min = min;
        region.bounding_box.max = max;
    };

    camera_module.each_camera(
        [&](render_id camera_id)
        {
            const auto& camera = camera_module.get_camera(camera_id);

            auto& clipmap = m_clipmaps[camera_id];
            if (clipmap.camera_id == INVALID_RENDER_ID)
            {
                return;
            }

            if (!camera.moved && !clipmap.need_clear)
            {
                return;
            }

            for (std::uint32_t i = 0; i < SDF_CLIPMAP_LEVEL_COUNT; ++i)
            {
                float page_extent = SDF_CLIPMAP_PAGE_EXTENT * static_cast<float>(1u << i);
                vec3i camera_page_coord = vector::floor(camera.position / page_extent);

                auto& clipmap_level = m_clipmap_levels[clipmap.level_offset + i];
                const vec3i& old_coord = clipmap_level.coord;

                if (camera_page_coord == old_coord)
                {
                    break;
                }

                vec3i window_min = camera_page_coord - half_page_size;
                vec3i window_max = camera_page_coord + half_page_size;

                vec3i delta = camera_page_coord - old_coord;

                bool full_update = old_coord.x == std::numeric_limits<std::int32_t>::max() &&
                                   old_coord.y == std::numeric_limits<std::int32_t>::max() &&
                                   old_coord.z == std::numeric_limits<std::int32_t>::max();

                for (std::uint32_t axis = 0; !full_update && axis < 3; ++axis)
                {
                    std::int32_t moved = delta[axis];
                    full_update = moved >= page_size || moved <= -page_size;
                }

                if (full_update)
                {
                    add_invalidation_box(
                        vec3f(window_min) * page_extent,
                        vec3f(window_max) * page_extent);
                }
                else
                {
                    for (std::uint32_t axis = 0; axis < 3; ++axis)
                    {
                        std::int32_t moved = delta[axis];
                        if (moved == 0)
                        {
                            continue;
                        }

                        vec3i slab_min = window_min;
                        vec3i slab_max = window_max;

                        if (moved > 0)
                        {
                            slab_min[axis] = window_max[axis] - moved;
                        }
                        else
                        {
                            slab_max[axis] = window_min[axis] - moved;
                        }

                        add_invalidation_box(
                            vec3f(slab_min) * page_extent,
                            vec3f(slab_max) * page_extent);
                    }
                }

                clipmap_level.coord = camera_page_coord;
                m_clipmap_levels.mark_dirty(clipmap.level_offset + i);
            }
        });

    m_clipmap_levels.update(
        [&](const clipmap_level_data& clipmap_level) -> clipmap_level_data::gpu_type
        {
            auto level_scale = static_cast<float>(1 << clipmap_level.level);
            vec3i origin = clipmap_level.coord - SDF_CLIPMAP_PAGE_COUNT_PER_AXIS / 2;

            vec3f voxel_extent =
                SDF_CLIPMAP_PAGE_EXTENT / SDF_CLIPMAP_UNIQUE_PAGE_RESOLUTION * level_scale;
            float max_distance = vector::length(voxel_extent) * SDF_MAX_DISTANCE_VOXEL_COUNT;

            return {
                .position = vec3f(origin) * SDF_CLIPMAP_PAGE_EXTENT * level_scale,
                .extent = SDF_CLIPMAP_EXTENT * level_scale,
                .max_distance = max_distance,
            };
        },
        [&](rhi_buffer* buffer, const void* data, std::size_t size, std::size_t offset)
        {
            rhi_buffer_region region = {
                .offset = offset,
                .size = size,
            };

            uploader.upload(
                buffer,
                data,
                size,
                region,
                RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_COMPUTE,
                RHI_ACCESS_SHADER_READ);
        });
}

void render_scene_sdf::update_meshes(gpu_buffer_uploader& uploader)
{
    auto* geometry_manager = render_device::instance().get_geometry_manager();

    m_meshes.update(
        [&](const mesh_data& mesh) -> mesh_data::gpu_type
        {
            box3f volume_bounds =
                geometry_manager->get_distance_field_bounds(mesh.distance_field_id);
            vec3f volume_center = box::get_center(volume_bounds);
            vec3f volume_extent = box::get_extent(volume_bounds);
            float max_extent = vector::max(volume_extent);

            mat4f volume_to_world = matrix::scale(vec3f(max_extent * 0.5f));
            volume_to_world = matrix::mul(volume_to_world, matrix::translation(volume_center));
            volume_to_world = matrix::mul(volume_to_world, mesh.matrix_m);

            vec4f volume_to_world_scale = {
                vector::length(volume_to_world[0]),
                vector::length(volume_to_world[1]),
                vector::length(volume_to_world[2]),
                0.0f,
            };
            volume_to_world_scale.w = std::min({
                volume_to_world_scale.x,
                volume_to_world_scale.y,
                volume_to_world_scale.z,
            });

            volume_bounds = box::transform(volume_bounds, mesh.matrix_m);

            return {
                .world_to_volume = matrix::inverse_transform(volume_to_world),
                .volume_to_world = volume_to_world,
                .volume_to_world_scale = volume_to_world_scale,
                .volume_bounds_min = volume_bounds.min,
                .distance_field_id = mesh.distance_field_id,
                .volume_bounds_max = volume_bounds.max,
            };
        },
        [&](rhi_buffer* buffer, const void* data, std::size_t size, std::size_t offset)
        {
            rhi_buffer_region region = {
                .offset = offset,
                .size = size,
            };

            uploader.upload(
                buffer,
                data,
                size,
                region,
                RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_COMPUTE,
                RHI_ACCESS_SHADER_READ);
        });
}

void render_scene_sdf::update_invalidation_regions(gpu_buffer_uploader& uploader)
{
    m_invalidation_regions.update(
        [](const invalidation_region& region) -> invalidation_region::gpu_type
        {
            return {
                .bounding_box_min = region.bounding_box.min,
                .bounding_box_max = region.bounding_box.max,
            };
        },
        [&](rhi_buffer* buffer, const void* data, std::size_t size, std::size_t offset)
        {
            rhi_buffer_region region = {
                .offset = offset,
                .size = size,
            };

            uploader.upload(
                buffer,
                data,
                size,
                region,
                RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_COMPUTE,
                RHI_ACCESS_SHADER_READ);
        });
}
} // namespace violet