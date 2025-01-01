#pragma once

#include "core/engine_system.hpp"
#include "math/types.hpp"

namespace violet
{
class transform_system : public engine_system
{
public:
    transform_system();

    bool initialize(const dictionary& config) override;

    void update_transform();

    mat4f get_local_matrix(entity e);
    mat4f get_world_matrix(entity e);

private:
    void update_local(bool force = false);
    void update_world(bool force = false);

    void update_world_recursive(entity e, const mat4f& parent_world, bool parent_dirty);

    std::uint32_t m_system_version{0};
};
} // namespace violet