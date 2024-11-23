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
    void update_render_data();

    std::uint32_t m_system_version{0};
};
} // namespace violet