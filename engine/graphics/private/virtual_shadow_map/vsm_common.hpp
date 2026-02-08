#pragma once

#include <cstdint>

namespace violet
{
static constexpr std::uint32_t MAX_SHADOW_LIGHT_COUNT = 32;
static constexpr std::uint32_t MAX_VSM_COUNT = 256;

static constexpr std::uint32_t VSM_VIRTUAL_PAGE_TABLE_SIZE = 64;
static constexpr std::uint32_t VSM_VIRTUAL_PAGE_TABLE_PAGE_COUNT =
    VSM_VIRTUAL_PAGE_TABLE_SIZE * VSM_VIRTUAL_PAGE_TABLE_SIZE;

static constexpr std::uint32_t VSM_PHYSICAL_PAGE_TABLE_SIZE = 32;
static constexpr std::uint32_t VSM_PHYSICAL_PAGE_TABLE_PAGE_COUNT =
    VSM_PHYSICAL_PAGE_TABLE_SIZE * VSM_PHYSICAL_PAGE_TABLE_SIZE;

static constexpr float VSM_PAGE_WORLD_SIZE = 1.28f * 2.0f / VSM_VIRTUAL_PAGE_TABLE_SIZE;
static constexpr std::uint32_t VSM_PAGE_RESOLUTION = 128;

static constexpr std::uint32_t VSM_VIRTUAL_RESOLUTION =
    VSM_VIRTUAL_PAGE_TABLE_SIZE * VSM_PAGE_RESOLUTION;
static constexpr std::uint32_t VSM_PHYSICAL_RESOLUTION =
    VSM_PHYSICAL_PAGE_TABLE_SIZE * VSM_PAGE_RESOLUTION;
} // namespace violet