#pragma once

#include "core/engine.hpp"
#include "graphics/resources/texture.hpp"

namespace violet
{
class imgui_system : public system
{
public:
    imgui_system();

    bool initialize(const dictionary& config) override;

private:
    void initialize_font();

    void begin_frame();
    void end_frame();

    std::unique_ptr<texture_2d> m_font;
};
} // namespace violet