#pragma once

#include "components/transform_component.hpp"
#include "core/engine.hpp"
#include "math/types.hpp"

namespace violet
{
class transform_system : public system
{
public:
    transform_system();

    void install(application& app) override;
    bool initialize(const dictionary& config) override;

    void update_transform();

    mat4f get_local_matrix(entity e);
    mat4f get_world_matrix(entity e);

private:
    void update_local(bool force = false);
    void update_world(bool force = false);

    void update_world_recursive(
        entity e,
        const transform_world_component& parent_world_transform,
        bool parent_dirty);

    std::uint32_t m_system_version{0};
};
} // namespace violet