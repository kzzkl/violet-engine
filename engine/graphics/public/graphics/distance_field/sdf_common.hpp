#pragma once

#include <cstdint>

namespace violet
{
static constexpr float SDF_CLIPMAP_EXTENT = 50.0f;

static constexpr std::uint32_t SDF_CLIPMAP_RESOLUTION = 252;
static constexpr std::uint32_t SDF_CLIPMAP_PAGE_RESOLUTION = 8;
static constexpr std::uint32_t SDF_CLIPMAP_UNIQUE_PAGE_RESOLUTION = SDF_CLIPMAP_PAGE_RESOLUTION - 1;

static constexpr std::uint32_t SDF_CLIPMAP_PAGE_COUNT_PER_AXIS =
    SDF_CLIPMAP_RESOLUTION / SDF_CLIPMAP_UNIQUE_PAGE_RESOLUTION;
static constexpr std::uint32_t SDF_CLIPMAP_PAGE_COUNT = SDF_CLIPMAP_PAGE_COUNT_PER_AXIS *
                                                        SDF_CLIPMAP_PAGE_COUNT_PER_AXIS *
                                                        SDF_CLIPMAP_PAGE_COUNT_PER_AXIS;
static constexpr float SDF_CLIPMAP_PAGE_EXTENT =
    SDF_CLIPMAP_EXTENT / SDF_CLIPMAP_PAGE_COUNT_PER_AXIS;

static constexpr std::uint32_t SDF_CLIPMAP_PAGES_PER_GRID_AXIS = 4;

static constexpr std::uint32_t SDF_CLIPMAP_GRID_COUNT_PER_AXIS =
    SDF_CLIPMAP_PAGE_COUNT_PER_AXIS / SDF_CLIPMAP_PAGES_PER_GRID_AXIS;
static constexpr std::uint32_t SDF_CLIPMAP_GRID_COUNT = SDF_CLIPMAP_GRID_COUNT_PER_AXIS *
                                                        SDF_CLIPMAP_GRID_COUNT_PER_AXIS *
                                                        SDF_CLIPMAP_GRID_COUNT_PER_AXIS;

static constexpr std::uint32_t SDF_CLIPMAP_AVERAGE_MESH_COUNT_PER_GRID = 512;

static constexpr std::uint32_t SDF_CLIPMAP_LEVEL_COUNT = 4;

static constexpr std::uint32_t SDF_CLIPMAP_PAGE_ATLAS_WIDTH = 1024;
static constexpr std::uint32_t SDF_CLIPMAP_PAGE_ATLAS_HEIGHT = 1024;

static constexpr std::uint32_t SDF_CLIPMAP_PAGE_ATLAS_PAGE_COUNT_X =
    SDF_CLIPMAP_PAGE_ATLAS_WIDTH / SDF_CLIPMAP_PAGE_RESOLUTION;
static constexpr std::uint32_t SDF_CLIPMAP_PAGE_ATLAS_PAGE_COUNT_Y =
    SDF_CLIPMAP_PAGE_ATLAS_HEIGHT / SDF_CLIPMAP_PAGE_RESOLUTION;

static constexpr float SDF_CLIPMAP_PAGE_ATLAS_OCCUPANCY = 0.3f;

static constexpr std::uint32_t SDF_MAX_MESH_COUNT = 1024 * 16;

static constexpr std::uint32_t SDF_BRICK_SIZE = 8;
static constexpr std::uint32_t SDF_BRICK_VOXEL_COUNT =
    SDF_BRICK_SIZE * SDF_BRICK_SIZE * SDF_BRICK_SIZE;
static constexpr std::uint32_t SDF_UNIQUE_BRICK_SIZE = SDF_BRICK_SIZE - 1;

static constexpr std::uint32_t SDF_BRICK_ATLAS_RESOLUTION = 1024;
static constexpr std::uint32_t SDF_BRICK_ATLAS_BRICK_COUNT_PER_AXIS =
    SDF_BRICK_ATLAS_RESOLUTION / SDF_BRICK_SIZE;

static constexpr std::uint32_t SDF_BRICK_ATLAS_BLOCK_COUNT_X =
    SDF_BRICK_ATLAS_BRICK_COUNT_PER_AXIS / 16;
static constexpr std::uint32_t SDF_BRICK_ATLAS_BLOCK_COUNT_Y = SDF_BRICK_ATLAS_BRICK_COUNT_PER_AXIS;

static constexpr std::uint32_t SDF_INVALID_BRICK = 0xFFFFFFFF;

static constexpr float SDF_MAX_DISTANCE_VOXEL_COUNT = 4.0f;
} // namespace violet