#pragma once

#include "core/engine_system.hpp"

namespace violet
{
class mesh_system : public engine_system
{
public:
    mesh_system();

    bool initialize(const dictionary& config) override;

private:
    void add_mesh_parameter();
    void remove_mesh_parameter();

    void update_mesh_parameter();

    std::uint32_t m_system_version{0};
};
} // namespace violet