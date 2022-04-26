#include "pmx_loader.hpp"
#include "encode.hpp"
#include "log.hpp"
#include <fstream>
#include <map>

namespace ash::sample::mmd
{
template <typename T>
static void read(std::istream& fin, T& dest)
{
    fin.read(reinterpret_cast<char*>(&dest), sizeof(T));
}

pmx_loader::pmx_loader()
{
    m_internal_toon = {
        "toon01.dds",
        "toon02.dds",
        "toon03.dds",
        "toon04.dds",
        "toon05.dds",
        "toon06.dds",
        "toon07.dds",
        "toon08.dds",
        "toon09.dds",
        "toon10.dds",
    };
}

bool pmx_loader::load(std::string_view path)
{
    std::ifstream fin(path.data(), std::ios::binary);
    if (!fin.is_open())
        return false;

    m_root_path = path;
    m_root_path = m_root_path.substr(0, m_root_path.find_last_of('/'));

    if (!load_header(fin))
        return false;

    if (!load_vertex(fin))
        return false;

    if (!load_index(fin))
        return false;

    if (!load_texture(fin))
        return false;

    if (!load_material(fin))
        return false;

    if (!load_bone(fin))
        return false;

    if (!load_morph(fin))
        return false;

    if (!load_display_frame(fin))
        return false;

    if (!load_rigidbody(fin))
        return false;

    if (!load_joint(fin))
        return false;

    return true;
}

std::vector<std::pair<std::size_t, std::size_t>> pmx_loader::submesh() const noexcept
{
    std::vector<std::pair<std::size_t, std::size_t>> result;
    result.reserve(m_materials.size());

    std::size_t offset = 0;
    for (auto& material : m_materials)
    {
        result.push_back(std::make_pair(offset, offset + material.num_indices));
        offset += material.num_indices;
    }

    return result;
}

bool pmx_loader::load_header(std::ifstream& fin)
{
    char magic[4];
    fin.read(magic, 4);

    read<float>(fin, m_header.version);

    std::uint8_t ignore;
    read<std::uint8_t>(fin, ignore); // ignore global size
    read<std::uint8_t>(fin, m_header.text_encoding);
    read<std::uint8_t>(fin, m_header.num_add_vec4);
    read<std::uint8_t>(fin, m_header.vertex_index_size);
    read<std::uint8_t>(fin, m_header.texture_index_size);
    read<std::uint8_t>(fin, m_header.material_index_size);
    read<std::uint8_t>(fin, m_header.bone_index_size);
    read<std::uint8_t>(fin, m_header.morph_index_size);
    read<std::uint8_t>(fin, m_header.rigidbody_index_size);

    m_header.name_jp = read_text(fin);
    m_header.name_en = read_text(fin);
    m_header.comments_jp = read_text(fin);
    m_header.comments_en = read_text(fin);

    return true;
}

bool pmx_loader::load_vertex(std::ifstream& fin)
{
    std::int32_t vertex_count;
    read<std::int32_t>(fin, vertex_count);

    m_vertices.resize(vertex_count);

    for (auto& v : m_vertices)
        v.add_uv.resize(m_header.num_add_vec4);

    for (pmx_vertex& vertex : m_vertices)
    {
        read<math::float3>(fin, vertex.position);
        read<math::float3>(fin, vertex.normal);
        read<math::float2>(fin, vertex.uv);

        for (std::uint8_t i = 0; i < m_header.num_add_vec4; ++i)
        {
            read<math::float4>(fin, vertex.add_uv[i]);
        }

        pmx_vertex_weight weight_type;
        read<pmx_vertex_weight>(fin, weight_type);

        float unused;
        switch (weight_type)
        {
        case pmx_vertex_weight::BDEF1: {
            vertex.bone.data[0] = read_index(fin, m_header.bone_index_size);
            vertex.weight.data[0] = 1.0f;
            break;
        }
        case pmx_vertex_weight::BDEF2: {
            vertex.bone.data[0] = read_index(fin, m_header.bone_index_size);
            vertex.bone.data[1] = read_index(fin, m_header.bone_index_size);
            read<float>(fin, vertex.weight.data[0]);
            vertex.weight.data[1] = 1.0f - vertex.weight.data[0];
            break;
        }
        case pmx_vertex_weight::BDEF4: {
            vertex.bone.data[0] = read_index(fin, m_header.bone_index_size);
            vertex.bone.data[1] = read_index(fin, m_header.bone_index_size);
            vertex.bone.data[2] = read_index(fin, m_header.bone_index_size);
            vertex.bone.data[3] = read_index(fin, m_header.bone_index_size);
            read<float>(fin, vertex.weight.data[0]);
            read<float>(fin, vertex.weight.data[1]);
            read<float>(fin, vertex.weight.data[2]);
            read<float>(fin, unused);
            break;
        }
        case pmx_vertex_weight::SDEF: {
            vertex.bone.data[0] = read_index(fin, m_header.bone_index_size);
            vertex.bone.data[1] = read_index(fin, m_header.bone_index_size);
            read<float>(fin, vertex.weight.data[0]);
            vertex.weight.data[1] = 1.0f - vertex.weight.data[0];

            math::float3 ignore;
            read<math::float3>(fin, ignore);
            read<math::float3>(fin, ignore);
            read<math::float3>(fin, ignore);
            break;
        }
        case pmx_vertex_weight::QDEF: {
            vertex.bone.data[0] = read_index(fin, m_header.bone_index_size);
            vertex.bone.data[1] = read_index(fin, m_header.bone_index_size);
            vertex.bone.data[2] = read_index(fin, m_header.bone_index_size);
            vertex.bone.data[3] = read_index(fin, m_header.bone_index_size);
            read<float>(fin, vertex.weight.data[0]);
            read<float>(fin, vertex.weight.data[1]);
            read<float>(fin, vertex.weight.data[2]);
            read<float>(fin, unused);
            break;
        }
        default:
            break;
        }

        read<float>(fin, vertex.edge_scale);
    }

    return true;
}

bool pmx_loader::load_index(std::ifstream& fin)
{
    std::int32_t num_index;
    read<std::int32_t>(fin, num_index);
    m_indices.reserve(num_index);

    for (std::int32_t i = 0; i < num_index; ++i)
        m_indices.push_back(read_index(fin, m_header.vertex_index_size));

    return true;
}

bool pmx_loader::load_texture(std::ifstream& fin)
{
    std::int32_t num_texture;
    read<std::int32_t>(fin, num_texture);

    m_textures.reserve(num_texture);
    for (std::int32_t i = 0; i < num_texture; ++i)
        m_textures.push_back(m_root_path + "/" + read_text(fin));

    return true;
}

bool pmx_loader::load_material(std::ifstream& fin)
{
    std::int32_t num_material;
    read<std::int32_t>(fin, num_material);

    m_materials.resize(num_material);

    for (auto& mat : m_materials)
    {
        mat.name_jp = read_text(fin);
        mat.name_en = read_text(fin);

        read<math::float4>(fin, mat.diffuse);
        read<math::float3>(fin, mat.specular);
        read<float>(fin, mat.specular_strength);
        read<math::float3>(fin, mat.ambient);
        read<draw_flag>(fin, mat.flag);
        read<math::float4>(fin, mat.edge);
        read<float>(fin, mat.edge_scale);
        mat.texture_index = read_index(fin, m_header.texture_index_size);
        mat.sphere_index = read_index(fin, m_header.texture_index_size);

        read<sphere_mode>(fin, mat.sphere_mode);
        read<toon_mode>(fin, mat.toon_mode);

        if (mat.toon_mode == toon_mode::TEXTURE)
        {
            mat.toon_index = read_index(fin, m_header.texture_index_size);
        }
        else if (mat.toon_mode == toon_mode::INTERNAL)
        {
            std::uint8_t toon_index;
            read<std::uint8_t>(fin, toon_index);
            mat.toon_index = toon_index;
        }
        else
        {
            return false;
        }

        mat.meta_data = read_text(fin);
        read<std::int32_t>(fin, mat.num_indices);
    }
    return true;
}

bool pmx_loader::load_bone(std::ifstream& fin)
{
    std::int32_t num_bone;
    read<std::int32_t>(fin, num_bone);

    m_bones.resize(num_bone);
    for (pmx_bone& bone : m_bones)
    {
        bone.name_jp = read_text(fin);
        bone.name_en = read_text(fin);

        read<math::float3>(fin, bone.position);
        bone.parent_index = read_index(fin, m_header.bone_index_size);
        read<std::int32_t>(fin, bone.layer);

        read<pmx_bone_flag>(fin, bone.flags);

        if (bone.flags & pmx_bone_flag::INDEXED_TAIL_POSITION)
            bone.tail_index = read_index(fin, m_header.bone_index_size);
        else
            read<math::float3>(fin, bone.tail_position);

        if (bone.flags & pmx_bone_flag::INHERIT_ROTATION ||
            bone.flags & pmx_bone_flag::INHERIT_TRANSLATION)
        {
            bone.inherit_index = read_index(fin, m_header.bone_index_size);
            read<float>(fin, bone.inherit_weight);
        }

        if (bone.flags & pmx_bone_flag::FIXED_AXIS)
            read<math::float3>(fin, bone.fixed_axis);

        if (bone.flags & pmx_bone_flag::LOCAL_AXIS)
        {
            read<math::float3>(fin, bone.local_x_axis);
            read<math::float3>(fin, bone.local_z_axis);
        }

        if (bone.flags & pmx_bone_flag::EXTERNAL_PARENT_DEFORM)
            bone.external_parent_index = read_index(fin, m_header.bone_index_size);

        if (bone.flags & pmx_bone_flag::IK)
        {
            bone.ik_target_index = read_index(fin, m_header.bone_index_size);
            read<std::int32_t>(fin, bone.ik_loop_count);
            read<float>(fin, bone.ik_limit);

            std::int32_t count;
            read<std::int32_t>(fin, count);
            bone.ik_links.resize(count);

            for (pmx_ik_link& link : bone.ik_links)
            {
                link.bone_index = read_index(fin, m_header.bone_index_size);

                unsigned char hasLimit;
                read<unsigned char>(fin, hasLimit);
                link.enable_limit = hasLimit != 0;

                if (link.enable_limit)
                {
                    read<math::float3>(fin, link.limit_min);
                    read<math::float3>(fin, link.limit_max);
                }
            }
        }
    }

    return true;
}

bool pmx_loader::load_morph(std::ifstream& fin)
{
    std::int32_t num_morph;
    read<std::int32_t>(fin, num_morph);

    m_morphs.resize(num_morph);

    for (pmx_morph& morph : m_morphs)
    {
        morph.name_jp = read_text(fin);
        morph.name_en = read_text(fin);

        read<std::uint8_t>(fin, morph.control_panel);
        read<pmx_morph_type>(fin, morph.type);

        std::int32_t count;
        read<std::int32_t>(fin, count);

        switch (morph.type)
        {
        case pmx_morph_type::GROUP:
            morph.group_morph.resize(count);
            for (auto& group : morph.group_morph)
            {
                group.index = read_index(fin, m_header.morph_index_size);
                read<float>(fin, group.weight);
            }
            break;
        case pmx_morph_type::VERTEX:
            morph.vertex_morph.resize(count);
            for (auto& vertex : morph.vertex_morph)
            {
                vertex.index = read_index(fin, m_header.vertex_index_size);
                read<math::float3>(fin, vertex.translation);
            }
            break;
        case pmx_morph_type::BONE:
            morph.bone_morph.resize(count);
            for (auto& bone : morph.bone_morph)
            {
                bone.index = read_index(fin, m_header.bone_index_size);
                read<math::float3>(fin, bone.translation);
                read<math::float4>(fin, bone.rotation);
            }
            break;

        case pmx_morph_type::UV:
        case pmx_morph_type::EXT_UV1:
        case pmx_morph_type::EXT_UV2:
        case pmx_morph_type::EXT_UV3:
        case pmx_morph_type::EXT_UV4:
            morph.uv_morph.resize(count);
            for (auto& uv : morph.uv_morph)
            {
                uv.index = read_index(fin, m_header.vertex_index_size);
                read<math::float4>(fin, uv.value);
            }
            break;

        case pmx_morph_type::MATERIAL:
            morph.material_morph.resize(count);
            for (auto& material : morph.material_morph)
            {
                material.index = read_index(fin, m_header.material_index_size);
                read<std::uint8_t>(fin, material.operate);
                read<math::float4>(fin, material.diffuse);
                read<math::float3>(fin, material.specular);
                read<float>(fin, material.specular_strength);
                read<math::float3>(fin, material.ambient);
                read<math::float4>(fin, material.edge_color);
                read<float>(fin, material.edge_scale);
                read<math::float4>(fin, material.tex_tint);
                read<math::float4>(fin, material.spa_tint);
                read<math::float4>(fin, material.toon_tint);
            }
            break;

        case pmx_morph_type::FLIP:
            morph.flip_morph.resize(count);
            for (auto& flip : morph.flip_morph)
            {
                flip.index = read_index(fin, m_header.morph_index_size);
                read<float>(fin, flip.weight);
            }
            break;

        case pmx_morph_type::IMPULSE:
            morph.impulse_morph.resize(count);
            for (auto& impulse : morph.impulse_morph)
            {
                impulse.index = read_index(fin, m_header.rigidbody_index_size);
                read<std::uint8_t>(fin, impulse.local_flag);
                read<math::float3>(fin, impulse.translate_velocity);
                read<math::float3>(fin, impulse.rotate_torque);
            }
            break;
        default:
            return false;
        }
    }

    return true;
}

bool pmx_loader::load_display_frame(std::ifstream& fin)
{
    std::int32_t numDisplayFrame;
    read<std::int32_t>(fin, numDisplayFrame);
    m_display_frames.resize(numDisplayFrame);

    for (pmx_display_data& display_frame : m_display_frames)
    {
        display_frame.name_jp = read_text(fin);
        display_frame.name_en = read_text(fin);

        read<std::uint8_t>(fin, display_frame.flag);

        std::int32_t numFrames;
        read<std::int32_t>(fin, numFrames);
        display_frame.frames.resize(numFrames);

        for (pmx_display_frame& frame : display_frame.frames)
        {
            read<pmx_frame_type>(fin, frame.type);
            switch (frame.type)
            {
            case pmx_frame_type::BONE:
                frame.index = read_index(fin, m_header.bone_index_size);
                break;
            case pmx_frame_type::MORPH:
                frame.index = read_index(fin, m_header.morph_index_size);
                break;
            default:
                return false;
            }
        }
    }

    return true;
}

bool pmx_loader::load_rigidbody(std::ifstream& fin)
{
    std::int32_t numRigidBody;
    read<std::int32_t>(fin, numRigidBody);
    m_rigidbodies.resize(numRigidBody);

    for (pmx_rigidbody& rigidbody : m_rigidbodies)
    {
        rigidbody.name_jp = read_text(fin);
        rigidbody.name_en = read_text(fin);

        rigidbody.bone_index = read_index(fin, m_header.bone_index_size);
        read<std::uint8_t>(fin, rigidbody.group);
        read<std::uint16_t>(fin, rigidbody.collision_group);

        read<pmx_rigidbody_shape_type>(fin, rigidbody.shape);
        read<math::float3>(fin, rigidbody.size);

        read<math::float3>(fin, rigidbody.translate);

        math::float3 rotate;
        read<math::float3>(fin, rotate);
        rigidbody.rotate = math::quaternion_plain::rotation_euler(rotate[1], rotate[0], rotate[2]);

        read<float>(fin, rigidbody.mass);
        read<float>(fin, rigidbody.translate_dimmer);
        read<float>(fin, rigidbody.rotate_dimmer);
        read<float>(fin, rigidbody.repulsion);
        read<float>(fin, rigidbody.friction);
        read<pmx_rigidbody_mode>(fin, rigidbody.mode);
    }

    return true;
}

bool pmx_loader::load_joint(std::ifstream& fin)
{
    std::int32_t numJoint;
    read<std::int32_t>(fin, numJoint);
    m_joints.resize(numJoint);

    for (pmx_joint& joint : m_joints)
    {
        joint.name_jp = read_text(fin);
        joint.name_en = read_text(fin);

        read<pmx_joint_type>(fin, joint.type);
        joint.rigidbody_a_index = read_index(fin, m_header.rigidbody_index_size);
        joint.rigidbody_b_index = read_index(fin, m_header.rigidbody_index_size);

        read<math::float3>(fin, joint.translate);
        math::float3 rotate;
        read<math::float3>(fin, rotate);
        joint.rotate = math::quaternion_plain::rotation_euler(rotate[1], rotate[0], rotate[2]);

        read<math::float3>(fin, joint.translate_min);
        read<math::float3>(fin, joint.translate_max);
        read<math::float3>(fin, joint.rotate_min);
        read<math::float3>(fin, joint.rotate_max);

        read<math::float3>(fin, joint.spring_translate_factor);
        read<math::float3>(fin, joint.spring_rotate_factor);
    }

    return true;
}

std::int32_t pmx_loader::read_index(std::ifstream& fin, std::uint8_t size)
{
    switch (size)
    {
    case 1: {
        std::uint8_t res;
        read<std::uint8_t>(fin, res);
        if (res != 0xFF)
            return static_cast<std::int32_t>(res);
        else
            return -1;
    }
    case 2: {
        std::uint16_t res;
        read<std::uint16_t>(fin, res);
        if (res != 0xFFFF)
            return static_cast<std::int32_t>(res);
        else
            return -1;
    }
    case 4: {
        std::int32_t res;
        read<std::int32_t>(fin, res);
        if (res != 0xFFFFFFFF)
            return static_cast<std::int32_t>(res);
        else
            return -1;
    }
    default:
        return 0;
    }
}

std::string pmx_loader::read_text(std::ifstream& fin)
{
    std::int32_t len;
    read<std::int32_t>(fin, len);
    if (len > 0)
    {
        std::string result;
        if (m_header.text_encoding == 1)
        {
            result.resize(len);
            fin.read(result.data(), len);
        }
        else if (m_header.text_encoding == 0)
        {
            std::u16string str;
            str.resize(len);
            fin.read(reinterpret_cast<char*>(str.data()), len);
            convert<encode_type::UTF16, encode_type::UTF8>(str, result);
        }
        return result;
    }
    else
    {
        return "";
    }
}
} // namespace ash::sample::mmd