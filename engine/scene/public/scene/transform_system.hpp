#pragma once

#include "core/engine_system.hpp"

namespace violet
{
class transform_system : public engine_system
{
public:
    transform_system();

    bool initialize(const dictionary& config) override;

private:
    void update_local();
    void update_world();

    std::uint32_t m_system_version;
};
} // namespace violet