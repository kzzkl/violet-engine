#pragma once

#include "graphics/rhi.hpp"
#include <memory>

namespace violet
{
/*class pipeline_parameter
{
public:
    pipeline_parameter(const rhi_pipeline_parameter_desc& desc);
    virtual ~pipeline_parameter() {}

    rhi_pipeline_parameter* get_interface() const noexcept { return m_interface.get(); }

protected:
    void set(std::size_t index, const void* data, size_t size);
    void set(std::size_t index, rhi_resource* texture);

private:
    std::unique_ptr<rhi_pipeline_parameter> m_interface;
};*/
} // namespace violet