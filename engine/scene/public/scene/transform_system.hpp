#pragma once

#include "core/engine_system.hpp"
#include "math/math.hpp"

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

    void update_world_recursive(entity e, const mat4f& parent_world, bool need_update);

    std::uint32_t m_system_version{0};
};
} // namespace violet