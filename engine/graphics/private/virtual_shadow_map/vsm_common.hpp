#pragma once

#include <cstdint>

namespace violet
{
static constexpr std::uint32_t MAX_SHADOW_LIGHT_COUNT = 32;
static constexpr std::uint32_t MAX_VSM_COUNT = 256;
static constexpr std::uint32_t VSM_PAGE_SIZE = 32;
static constexpr std::uint32_t VSM_PAGE_COUNT = VSM_PAGE_SIZE * VSM_PAGE_SIZE;
static constexpr std::uint32_t VSM_PAGE_PIXEL_SIZE = 128;
static constexpr std::uint32_t VSM_PAGE_PIXEL_COUNT = VSM_PAGE_PIXEL_SIZE * VSM_PAGE_PIXEL_SIZE;
static constexpr std::uint32_t VSM_PHYSICAL_PAGE_SIZE = 32;
static constexpr std::uint32_t VSM_PHYSICAL_PAGE_COUNT =
    VSM_PHYSICAL_PAGE_SIZE * VSM_PHYSICAL_PAGE_SIZE;
} // namespace violet