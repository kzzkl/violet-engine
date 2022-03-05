#pragma once

#include "graphics_interface.hpp"
#include <string>
#include <vector>

namespace ash::graphics::d3d12
{
struct d3d12_adapter_info
{
    std::string description;
};

class d3d12_diagnotor : public ash::graphics::external::diagnotor
{
public:
    using diagnotor_config = ash::graphics::external::graphics_context_config;

public:
    void initialize(const diagnotor_config& config);

    virtual int get_adapter_info(ash::graphics::external::adapter_info* infos, int size) override;

private:
    std::vector<d3d12_adapter_info> m_adapter_info;
};
} // namespace ash::graphics::d3d12