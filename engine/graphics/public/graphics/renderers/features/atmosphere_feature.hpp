#pragma once

#include "graphics/renderer.hpp"

namespace violet
{
class atmosphere_feature : public render_feature<atmosphere_feature>
{
public:
    void set_use_multi_scattering(bool use_multi_scattering) noexcept
    {
        m_use_multi_scattering = use_multi_scattering;
    }

    bool get_use_multi_scattering() const noexcept
    {
        return m_use_multi_scattering;
    }

    void set_ibl_update_interval(std::uint32_t ibl_update_interval) noexcept
    {
        m_ibl_update_interval = ibl_update_interval;
    }

    std::uint32_t get_ibl_update_interval() const noexcept
    {
        return m_ibl_update_interval;
    }

private:
    bool m_use_multi_scattering{true};
    std::uint32_t m_ibl_update_interval{10};
};
} // namespace violet