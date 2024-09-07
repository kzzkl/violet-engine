#pragma once

#include "core/engine_system.hpp"

namespace violet
{
class hierarchy_system : public engine_system
{
public:
    hierarchy_system();

    bool initialize(const dictionary& config) override;

private:
    void add_previous_parent();
    void add_child();
    void update_child();

    std::uint32_t m_system_version{0};
};
} // namespace violet