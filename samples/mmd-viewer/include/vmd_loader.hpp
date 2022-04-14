#pragma once

#include "math.hpp"
#include <cstddef>
#include <fstream>
#include <string>
#include <vector>

namespace ash::sample::mmd
{
struct vmd_header
{
    std::string version;
    std::string model_name;
};

struct vmd_motion
{
    std::string bone_name;
    std::uint32_t frame_index;
    math::float3 translate;
    math::float4 rotate;
    std::uint8_t interpolation[64];
};

class vmd_loader
{
public:
    bool load(std::string_view path);

    const std::vector<vmd_motion> motions() const noexcept { return m_motions; }

private:
    bool load_header(std::ifstream& fin);
    bool load_motion(std::ifstream& fin);

    vmd_header m_header;
    std::vector<vmd_motion> m_motions;
};
} // namespace ash::sample::mmd