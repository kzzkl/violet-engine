#pragma once

namespace violet
{
struct first_person_control_component
{
    float move_speed{5.0f};
    float yaw_speed{1.0f};
    float pitch_speed{0.5f};

    float yaw{0.0f};
    float pitch{0.0f};
};
} // namespace violet