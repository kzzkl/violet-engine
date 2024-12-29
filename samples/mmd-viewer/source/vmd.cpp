#include "vmd.hpp"
#include "common/utility.hpp"
#include "encode.hpp"

namespace violet
{
template <typename T>
static void read(std::istream& fin, T& dest)
{
    fin.read(reinterpret_cast<char*>(&dest), sizeof(T));
}

vmd::vmd() = default;

bool vmd::load(std::string_view path)
{
    std::wstring path_wstring = string_to_wstring(path);

    std::ifstream fin(path_wstring.data(), std::ios::binary);
    if (!fin.is_open())
    {
        return false;
    }

    load_header(fin);
    load_motion(fin);

    if (!fin.eof())
    {
        load_morph(fin);
    }

    if (!fin.eof())
    {
        load_camera(fin);
    }

    if (!fin.eof())
    {
        load_light(fin);
    }

    if (!fin.eof())
    {
        load_shadow(fin);
    }

    if (!fin.eof())
    {
        load_ik(fin);
    }

    return true;
}

void vmd::load_header(std::ifstream& fin)
{
    char version[30] = {};
    fin.read(version, 30);
    header.version = version;

    char model_name[20] = {};
    fin.read(model_name, 20);
    header.model_name = model_name;
}

void vmd::load_motion(std::ifstream& fin)
{
    std::uint32_t size;
    read(fin, size);
    motions.reserve(size);

    for (std::uint32_t i = 0; i < size; ++i)
    {
        vmd_motion motion;

        char bone_name[15] = {};
        read(fin, bone_name);
        convert<ENCODE_TYPE_SHIFT_JIS, ENCODE_TYPE_UTF8>(bone_name, motion.bone_name);

        read(fin, motion.frame_index);
        read(fin, motion.translate);
        read(fin, motion.rotate);
        read(fin, motion.interpolation);

        motions.push_back(motion);
    }
}

void vmd::load_morph(std::ifstream& fin)
{
    std::uint32_t size;
    read(fin, size);
    motions.reserve(size);

    for (std::uint32_t i = 0; i < size; ++i)
    {
        vmd_morph morph;

        char name[15] = {};
        read(fin, name);
        convert<ENCODE_TYPE_SHIFT_JIS, ENCODE_TYPE_UTF8>(name, morph.morph_name);

        read(fin, morph.frame);
        read(fin, morph.weight);
        morphs.push_back(morph);
    }
}

void vmd::load_camera(std::ifstream& fin)
{
    std::uint32_t size;
    read(fin, size);
    cameras.reserve(size);

    for (std::uint32_t i = 0; i < size; ++i)
    {
        vmd_camera camera;
        read(fin, camera.frame);
        read(fin, camera.distance);
        read(fin, camera.interest);
        read(fin, camera.rotate);
        read(fin, camera.interpolation);
        read(fin, camera.view_angle);
        read(fin, camera.is_perspective);
        cameras.push_back(camera);
    }
}

void vmd::load_light(std::ifstream& fin)
{
    std::uint32_t size;
    read(fin, size);
    lights.reserve(size);

    for (std::uint32_t i = 0; i < size; ++i)
    {
        vmd_light light;
        read(fin, light.frame);
        read(fin, light.color);
        read(fin, light.position);
        lights.push_back(light);
    }
}

void vmd::load_shadow(std::ifstream& fin)
{
    std::uint32_t size;
    read(fin, size);
    lights.reserve(size);

    for (std::uint32_t i = 0; i < size; ++i)
    {
        vmd_shadow shadow;
        read(fin, shadow.frame);
        read(fin, shadow.shadow_type);
        read(fin, shadow.distance);
        shadows.push_back(shadow);
    }
}

void vmd::load_ik(std::ifstream& fin)
{
    std::uint32_t size;
    read(fin, size);
    lights.reserve(size);

    for (std::uint32_t i = 0; i < size; ++i)
    {
        vmd_ik ik;
        read(fin, ik.frame);
        read(fin, ik.show);

        std::uint32_t info_size;
        read(fin, info_size);
        ik.infos.reserve(info_size);
        for (std::uint32_t i = 0; i < info_size; ++i)
        {
            vmd_ik_info info;
            char name[20] = {};
            read(fin, name);
            convert<ENCODE_TYPE_SHIFT_JIS, ENCODE_TYPE_UTF8>(name, info.name);
            read(fin, info.enable);
            ik.infos.push_back(info);
        }

        iks.push_back(ik);
    }
}
} // namespace violet