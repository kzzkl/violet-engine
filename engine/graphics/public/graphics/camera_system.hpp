#pragma once

#include "core/engine_system.hpp"

namespace violet
{
class camera_system : public engine_system
{
public:
    camera_system();

    bool initialize(const dictionary& config) override;

private:
    void add_camera_parameter();
    void remove_camera_parameter();

    void update_camera_parameter();

    std::uint32_t m_system_version{0};
};
} // namespace violet