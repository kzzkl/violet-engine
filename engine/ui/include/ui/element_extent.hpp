#pragma once

namespace ash::ui
{
struct element_extent
{
    float x;
    float y;
    float width;
    float height;

    bool operator==(const element_extent& other) const noexcept
    {
        return x == other.x && y == other.y && width == other.width && height == other.height;
    }

    bool operator!=(const element_extent& other) const noexcept { return !operator==(other); }
};
} // namespace ash::ui