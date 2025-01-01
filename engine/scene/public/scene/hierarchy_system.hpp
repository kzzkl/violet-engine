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
    void process_add_parent();
    void process_set_parent();
    void process_remove_parent();

    std::uint32_t m_system_version{0};
};
} // namespace violet