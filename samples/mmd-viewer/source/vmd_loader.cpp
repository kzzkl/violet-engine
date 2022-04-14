#include "vmd_loader.hpp"
#include "encode.hpp"

namespace ash::sample::mmd
{
template <typename T>
static void read(std::istream& fin, T& dest)
{
    fin.read(reinterpret_cast<char*>(&dest), sizeof(T));
}

bool vmd_loader::load(std::string_view path)
{
    std::ifstream fin(path.data(), std::ios::binary);
    if (!fin.is_open())
        return false;

    if (!load_header(fin))
        return false;
    if (!load_motion(fin))
        return false;

    return true;
}

bool vmd_loader::load_header(std::ifstream& fin)
{
    char version[30] = {};
    fin.read(version, 30);
    m_header.version = version;

    char model_name[20] = {};
    fin.read(model_name, 20);
    m_header.model_name = model_name;

    return true;
}

bool vmd_loader::load_motion(std::ifstream& fin)
{
    std::uint32_t size;
    read(fin, size);

    m_motions.reserve(size);
    for (std::uint32_t i = 0; i < size; ++i)
    {
        vmd_motion motion;

        char bone_name[15] = {};
        read(fin, bone_name);
        convert<encode_type::SHIFT_JIS, encode_type::UTF8>(bone_name, motion.bone_name);

        read(fin, motion.frame_index);
        read(fin, motion.position);
        read(fin, motion.rotation);
        read(fin, motion.interpolation);

        m_motions.push_back(motion);
    }

    return true;
}
} // namespace ash::sample::mmd