#pragma once

#include "math/math.hpp"
#include <array>
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
    std::array<std::uint8_t, 64> interpolation;
};

struct vmd_morph
{
    std::string morph_name;
    std::uint32_t frame;
    float weight;
};

struct vmd_camera
{
    std::uint32_t frame;
    float distance;
    math::float3 interest;
    math::float3 rotate;
    std::array<std::uint8_t, 24> interpolation;
    std::uint32_t view_angle;
    std::uint8_t is_perspective;
};

struct vmd_light
{
    std::uint32_t frame;
    math::float3 color;
    math::float3 position;
};

struct vmd_shadow
{
    std::uint32_t frame;
    std::uint8_t shadow_type; // 0:Off 1:mode1 2:mode2
    float distance;
};

struct vmd_ik_info
{
    std::string name;
    std::uint8_t enable;
};

struct vmd_ik
{
    std::uint32_t frame;
    std::uint8_t show;
    std::vector<vmd_ik_info> infos;
};

class vmd_loader
{
public:
    bool load(std::string_view path);

    const std::vector<vmd_motion> motions() const noexcept { return m_motions; }
    const std::vector<vmd_morph> morphs() const noexcept { return m_morphs; }
    const std::vector<vmd_ik> iks() const noexcept { return m_iks; }

private:
    void load_header(std::ifstream& fin);
    void load_motion(std::ifstream& fin);
    void load_morph(std::ifstream& fin);
    void load_camera(std::ifstream& fin);
    void load_light(std::ifstream& fin);
    void load_shadow(std::ifstream& fin);
    void load_ik(std::ifstream& fin);

    vmd_header m_header;
    std::vector<vmd_motion> m_motions;
    std::vector<vmd_morph> m_morphs;
    std::vector<vmd_camera> m_cameras;
    std::vector<vmd_light> m_lights;
    std::vector<vmd_shadow> m_shadows;
    std::vector<vmd_ik> m_iks;
};
} // namespace ash::sample::mmd