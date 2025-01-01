#pragma once

#include "math/types.hpp"
#include <array>
#include <fstream>
#include <string>
#include <vector>

namespace violet
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
    vec3f translate;
    vec4f rotate;
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
    vec3f interest;
    vec3f rotate;
    std::array<std::uint8_t, 24> interpolation;
    std::uint32_t view_angle;
    std::uint8_t is_perspective;
};

struct vmd_light
{
    std::uint32_t frame;
    vec3f color;
    vec3f position;
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

class vmd
{
public:
    vmd();

    bool load(std::string_view path);

    vmd_header header;
    std::vector<vmd_motion> motions;
    std::vector<vmd_morph> morphs;
    std::vector<vmd_camera> cameras;
    std::vector<vmd_light> lights;
    std::vector<vmd_shadow> shadows;
    std::vector<vmd_ik> iks;

private:
    void load_header(std::ifstream& fin);
    void load_motion(std::ifstream& fin);
    void load_morph(std::ifstream& fin);
    void load_camera(std::ifstream& fin);
    void load_light(std::ifstream& fin);
    void load_shadow(std::ifstream& fin);
    void load_ik(std::ifstream& fin);
};
} // namespace violet