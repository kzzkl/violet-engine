#pragma once

#include "common/type_index.hpp"
#include "resource_registry.hpp"
#include <memory>
#include <vector>

namespace violet::resource
{
struct resource_index : public type_index<resource_index, std::size_t>
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
} // namespace violet::resource