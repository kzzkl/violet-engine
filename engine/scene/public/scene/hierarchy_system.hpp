#pragma once

#include "core/engine.hpp"

namespace violet
{
class hierarchy_system : public system
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