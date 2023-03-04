#pragma once

#include "ui/control.hpp"

namespace violet::ui
{
class dock_area;
class dock_node : public control
{
public:
    dock_node(dock_area* area);
    virtual ~dock_node() = default;

    std::pair<float, float> dock_extent() const noexcept { return {m_width, m_height}; }
    void dock_width(float value) noexcept;
    void dock_height(float value) noexcept;

protected:
    dock_area* m_dock_area;

private:
    friend class dock_area;

    float m_width;
    float m_height;

    std::shared_ptr<dock_node> m_dock_parent;
};
} // namespace violet::ui