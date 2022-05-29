#pragma once

#include "index_generator.hpp"
#include "resource_registry.hpp"
#include <memory>
#include <vector>

namespace ash::resource
{
struct resource_index : public index_generator<resource_index, std::size_t>
{
};

class resource_manager
{
};

class resource
{
public:
private:
    // std::vector<std::unique_ptr<dispatcher>> m_managers;
};
} // namespace ash::resource