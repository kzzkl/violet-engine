#pragma once

#include "core/context/engine_module.hpp"
#include "graphics/rhi/rhi.hpp"

namespace violet
{
class graphics : public engine_module
{
public:
    graphics();
    virtual ~graphics();

    virtual bool initialize(const dictionary& config) override;

    rhi& get_rhi() const { return *m_rhi; }

private:
    std::unique_ptr<rhi> m_rhi;
};
} // namespace violet