#include "graphics/render_scene/render_scene_sdf.hpp"
#include "distance_field/distance_field_manager.hpp"
#include "gpu_buffer_uploader.hpp"
#include "graphics/render_scene/render_scene_camera.hpp"
#include "math/vector.hpp"
#include "surface_cache/surface_cache_manager.hpp"

namespace violet
{
render_scene_sdf::render_scene_sdf()
    : m_clipmap_levels(128),
      m_surface_caches(16 * 1024 * 1024)
{
}

render_id render_scene_sdf::add_mesh()
{
    return m_meshes.add();
}

void render_scene_sdf::set_mesh(
    render_id mesh_sdf_id,
    geometry* geometry,
    std::span<std::pair<std::uint32_t, material*>> materials)
{
    auto& mesh = m_meshes[mesh_sdf_id];
    mesh.geometry = geometry;
    mesh.materials.assign(materials.begin(), materials.end());

    auto* surface_cache_manager = render_device::instance().get_surface_cache_manager();
    if (mesh.surface_cache_id != INVALID_RENDER_ID)
    {
        surface_cache_manager->remove_surface_cache(mesh.surface_cache_id);
    }

    mesh.surface_cache_id = surface_cache_manager->add_surface_cache(128, 128, geometry, materials);

    m_meshes.mark_dirty(mesh_sdf_id);
}

void render_scene_sdf::set_mesh_matrix(
    render_id mesh_sdf_id,
    const mat4f& matrix_m,
    const vec3f& scale)
{
    auto& mesh = m_meshes[mesh_sdf_id];
    mesh.matrix_m = matrix_m;
    mesh.scale = vector::max(scale);
    m_meshes.mark_dirty(mesh_sdf_id);
}

void render_scene_sdf::remove_mesh(render_id mesh_sdf_id)
{
    auto& mesh = m_meshes[mesh_sdf_id];

    if (mesh.surface_cache_id != INVALID_RENDER_ID)
    {
        auto* surface_cache_manager = render_device::instance().get_surface_cache_manager();
        surface_cache_manager->remove_surface_cache(mesh.surface_cache_id);
    }

    m_meshes.remove(mesh_sdf_id);
}

void render_scene_sdf::upload(render_scene_context& context, gpu_buffer_uploader& uploader)
{
    deallocate_clipmaps(context);
    allocate_clipmaps(context);
    update_clipmap(context, uploader);

    update_meshes(uploader);
    update_invalidation_regions(uploader);
}

void render_scene_sdf::update(render_scene_context& context, render_graph& graph) {}

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
                auto level_scale = static_cast<float>(1 << i);

                vec3f page_extent = SDF_CLIPMAP_PAGE_EXTENT * level_scale;
                float page_diagonal = vector::length(page_extent);

                vec3f voxel_extent =
                    SDF_CLIPMAP_PAGE_EXTENT / SDF_CLIPMAP_UNIQUE_PAGE_RESOLUTION * level_scale;
                float max_distance = vector::length(voxel_extent) * SDF_MAX_DISTANCE_VOXEL_COUNT;

                m_clipmap_levels[i + clipmap.level_offset] = {
                    .center = std::numeric_limits<std::int32_t>::max(),
                    .level = i,
                    .extent = SDF_CLIPMAP_EXTENT * level_scale,
                    .page_extent = page_extent.x,
                    .page_diagonal = page_diagonal,
                    .voxel_extent = page_extent.x / SDF_CLIPMAP_UNIQUE_PAGE_RESOLUTION,
                    .max_distance = max_distance,
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

                if (camera_page_coord == clipmap_level.center)
                {
                    break;
                }

                vec3i window_min = camera_page_coord - half_page_size;
                vec3i window_max = camera_page_coord + half_page_size;

                vec3i delta = camera_page_coord - clipmap_level.center;

                bool full_update =
                    clipmap_level.center.x == std::numeric_limits<std::int32_t>::max() &&
                    clipmap_level.center.y == std::numeric_limits<std::int32_t>::max() &&
                    clipmap_level.center.z == std::numeric_limits<std::int32_t>::max();

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

                clipmap_level.center = camera_page_coord;
                m_clipmap_levels.mark_dirty(clipmap.level_offset + i);
            }
        });

    m_clipmap_levels.update(
        [&](const clipmap_level_data& clipmap_level) -> clipmap_level_data::gpu_type
        {
            vec3i origin = clipmap_level.center - SDF_CLIPMAP_PAGE_COUNT_PER_AXIS / 2;

            std::int32_t n = SDF_CLIPMAP_PAGE_COUNT_PER_AXIS;
            vec3i origin_wrapped = {
                ((origin.x % n) + n) % n,
                ((origin.y % n) + n) % n,
                ((origin.z % n) + n) % n,
            };

            return {
                .position = vec3f(origin) * clipmap_level.page_extent,
                .level = clipmap_level.level,
                .origin_wrapped = origin_wrapped,
                .extent = clipmap_level.extent,
                .page_extent = clipmap_level.page_extent,
                .page_diagonal = clipmap_level.page_diagonal,
                .voxel_extent = clipmap_level.voxel_extent,
                .max_distance = clipmap_level.max_distance,
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

void render_scene_sdf::update_surface_caches(gpu_buffer_uploader& uploader) {}

void render_scene_sdf::update_meshes(gpu_buffer_uploader& uploader)
{
    auto* distance_field_manager = render_device::instance().get_distance_field_manager();

    m_meshes.update(
        [&](const mesh_data& mesh) -> mesh_data::gpu_type
        {
            box3f volume_bounds =
                distance_field_manager->get_volume_bounds(mesh.geometry->get_distance_field_id());
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
                .distance_field_id = mesh.geometry->get_distance_field_id(),
                .volume_bounds_max = volume_bounds.max,
                .surface_cache_id = mesh.surface_cache_id,
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