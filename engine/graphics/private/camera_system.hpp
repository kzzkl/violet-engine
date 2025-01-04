#pragma once

#include "core/engine.hpp"

namespace violet
{
class camera_system : public system
{
public:
    camera_system();

    bool initialize(const dictionary& config) override;

    void update();

private:
    std::uint32_t m_system_version{0};
};
} // namespace violet